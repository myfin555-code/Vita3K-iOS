// Vita3K emulator project
// Copyright (C) 2026 Vita3K team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#pragma once

#include <cstdint>

namespace mem {
// Guest allocation accounting, not process RSS or virtual address-space size.
// Serialized by MemState::generation_mutex in the allocator.
class AllocationBudget {
    uint64_t limit_bytes = uint64_t{ 1 } << 32;
    uint64_t used_bytes = 0;

public:
    void reset(uint64_t limit) {
        limit_bytes = limit;
        used_bytes = 0;
    }
    bool reserve(uint64_t bytes) {
        if (bytes > remaining())
            return false;
        used_bytes += bytes;
        return true;
    }
    void release(uint64_t bytes) { used_bytes = bytes > used_bytes ? 0 : used_bytes - bytes; }
    uint64_t remaining() const { return limit_bytes - used_bytes; }
    uint64_t used() const { return used_bytes; }
};
} // namespace mem
