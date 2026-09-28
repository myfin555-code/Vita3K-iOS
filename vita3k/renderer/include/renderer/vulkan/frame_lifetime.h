// Vita3K emulator project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

namespace renderer::vulkan {

constexpr int MAX_FRAMES_RENDERING = 3;

// Subtract only after checking ordering: startup frames are smaller than the window.
constexpr bool is_frame_timestamp_in_flight(uint64_t timestamp, uint64_t current) {
    return timestamp != ~uint64_t{ 0 } && timestamp <= current
        && current - timestamp < MAX_FRAMES_RENDERING;
}

} // namespace renderer::vulkan
