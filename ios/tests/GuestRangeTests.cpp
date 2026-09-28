#include <mem/guest_range.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

static void require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

int main() {
    std::array<bool, 4> allocated{ false, true, true, true };
    auto page = [&](uint32_t address) {
        return address / 4096 < allocated.size() && allocated[address / 4096];
    };
    require(mem::valid_guest_range(4096, 3 * 4096, page), "all allocated pages must be readable");
    allocated[2] = false; // movie buffer freed before the queued texture bind
    require(!mem::valid_guest_range(4096, 3 * 4096, page), "a hole between valid endpoints must reject the entire read");
    require(mem::valid_guest_range(4096, 4096, page), "exclusive end must not inspect the next page");
    require(!mem::valid_guest_range(8191, 2, page), "an unaligned read crossing into a freed page must fail");
    require(!mem::valid_guest_range(0, 4, page), "null address must fail");
    require(!mem::valid_guest_range(4096, 0, page), "empty texture must fail");

    unsigned checks = 0;
    auto last_page = [&](uint32_t address) {
        ++checks;
        return address == 0xfffff000U;
    };
    require(mem::valid_guest_range(0xffffffffULL, 1, last_page), "last guest byte must not wrap the end");
    require(checks == 1, "last page must be checked exactly once");
    require(!mem::valid_guest_range(0xffffffffULL, 2, last_page), "32-bit range overflow must fail");
    require(!mem::valid_guest_range(0x100000000ULL, 1, last_page), "out of range address must fail");
    require(!mem::valid_guest_range(4096, std::numeric_limits<uint64_t>::max(), last_page), "64-bit length overflow must fail");
    require(checks == 1, "overflow cases must be rejected before checking pages");
    std::cout << "Guest texture memory range checks passed\n";
}
