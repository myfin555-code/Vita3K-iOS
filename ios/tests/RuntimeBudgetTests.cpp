#include <cpu/slot_pool.h>
#include <mem/allocation_budget.h>

#include <atomic>
#include <cstdlib>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

static void require(bool value) {
    if (!value)
        std::abort();
}

int main() {
    mem::AllocationBudget budget;
    budget.reset(640ULL * 1024 * 1024);
    require(budget.reserve(512ULL * 1024 * 1024));
    require(budget.reserve(128ULL * 1024 * 1024));
    require(!budget.reserve(1));
    require(!budget.reserve(UINT64_MAX));
    budget.release(128ULL * 1024 * 1024);
    require(budget.remaining() == 128ULL * 1024 * 1024);
    require(budget.reserve(4096));
    budget.reset(512ULL * 1024 * 1024);
    require(budget.used() == 0);

    for (const std::size_t slots : { 1, 3, 37 }) {
        cpu::SlotPool pool(slots);
        std::atomic_bool cancelled{ false };
        std::atomic_int active{ 0 }, completed{ 0 };
        std::vector<std::thread> threads;
        for (int i = 0; i < 48; ++i) {
            threads.emplace_back([&] {
                for (int n = 0; n < 50; ++n) {
                    const auto id = pool.acquire(cancelled);
                    require(id.has_value());
                    require(++active <= static_cast<int>(slots));
                    std::this_thread::yield();
                    --active;
                    ++completed;
                    pool.release(*id);
                }
            });
        }
        for (auto &thread : threads)
            thread.join();
        require(active == 0 && completed == 2400);
    }
    // Cancel a waiting guest without requiring the busy slot to become free.
    cpu::SlotPool pool(1);
    std::atomic_bool no{ false }, cancelled{ false };
    const auto held = pool.acquire(no);
    auto waiter = std::async(std::launch::async, [&] { return pool.acquire(cancelled); });
    cancelled.store(true);
    pool.wake();
    require(waiter.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
    require(!waiter.get());
    pool.release(*held);
    const auto next = pool.acquire(no);
    require(next.has_value());
    pool.release(*next);
    std::cout << "JIT slot admission, cancellation and guest RAM budget passed\n";
}
