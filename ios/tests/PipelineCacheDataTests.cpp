#include <renderer/vulkan/pipeline_cache_data.h>

#include <array>
#include <cstdlib>
#include <iostream>

static void require(bool value) {
    if (!value)
        std::abort();
}

int main() {
    // Unaligned input, with explicit little-endian Vulkan header words.
    std::array<uint8_t, 34> storage{};
    auto *data = storage.data() + 1;
    data[0] = 32;
    data[4] = 1;
    data[8] = 0x34;
    data[9] = 0x12;
    data[12] = 0x78;
    data[13] = 0x56;
    std::array<uint8_t, 16> uuid{};
    for (int i = 0; i < 16; ++i)
        data[16 + i] = uuid[i] = i + 1;
    const auto valid = [&](std::size_t size) {
        return renderer::vulkan::compatible_pipeline_cache(data, size, 0x1234, 0x5678, uuid.data());
    };
    require(valid(32));
    require(valid(33)); // Driver-specific payload follows the header.
    for (std::size_t size = 0; size < 32; ++size)
        require(!valid(size));
    require(!renderer::vulkan::compatible_pipeline_cache(nullptr, 32, 0x1234, 0x5678, uuid.data()));
    require(!renderer::vulkan::compatible_pipeline_cache(data, 32, 0x1234, 0x5678, nullptr));
    data[16] ^= 1;
    require(!valid(32));
    data[16] ^= 1; // Old driver UUID.
    data[8] ^= 1;
    require(!valid(32));
    data[8] ^= 1; // Another vendor.
    data[12] ^= 1;
    require(!valid(32));
    data[12] ^= 1; // Another GPU.
    data[4] = 2;
    require(!valid(32));
    data[4] = 1; // Unknown header format.
    data[0] = 31;
    require(!valid(32));
    data[0] = 33;
    require(!valid(32)); // Header extends past end of data.
    data[0] = data[1] = data[2] = data[3] = 255;
    require(!valid(32));
    std::cout << "Pipeline cache GPU/driver compatibility and truncated-header checks passed\n";
}
