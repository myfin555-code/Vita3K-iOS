#pragma once

#include <cstdint>

namespace mem {
// Validate every 4 KiB guest page without truncating a 64-bit end address.
// The caller must hold the allocation lock through validation AND the read.
template <typename IsAllocated>
bool valid_guest_range(uint64_t address, uint64_t size, IsAllocated is_allocated) {
    constexpr uint64_t address_space_size = uint64_t{ 1 } << 32;
    constexpr uint64_t page_size = 4096;
    if (!address || !size || address >= address_space_size || size > address_space_size - address)
        return false;
    const uint64_t last_page = (address + size - 1) / page_size;
    for (uint64_t page = address / page_size; page <= last_page; ++page) {
        if (!is_allocated(static_cast<uint32_t>(page * page_size)))
            return false;
    }
    return true;
}
} // namespace mem
