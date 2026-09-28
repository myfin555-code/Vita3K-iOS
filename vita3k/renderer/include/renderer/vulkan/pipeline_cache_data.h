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

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace renderer::vulkan {
// VkPipelineCacheHeaderVersionOne is serialized little-endian, independent of
// host alignment. Reject an old driver/GPU cache before giving its blob to Vulkan.
inline bool compatible_pipeline_cache(const void *data, std::size_t size,
    uint32_t vendor, uint32_t device, const uint8_t *uuid) {
    constexpr std::size_t header_size = 4 * sizeof(uint32_t) + 16;
    if (!data || !uuid || size < header_size)
        return false;
    const auto *bytes = static_cast<const uint8_t *>(data);
    const auto word = [&](std::size_t offset) {
        return uint32_t(bytes[offset]) | (uint32_t(bytes[offset + 1]) << 8)
            | (uint32_t(bytes[offset + 2]) << 16) | (uint32_t(bytes[offset + 3]) << 24);
    };
    return word(0) >= header_size && word(0) <= size && word(4) == 1
        && word(8) == vendor && word(12) == device
        && std::memcmp(bytes + 16, uuid, 16) == 0;
}
} // namespace renderer::vulkan
