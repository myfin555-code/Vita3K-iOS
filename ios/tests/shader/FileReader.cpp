// SPDX-License-Identifier: GPL-2.0-or-later
// The shader CLI's optional file reader uses std::ifstream in this host harness,
// avoiding SDL startup. Shader/GXM/compiler code and logging are the real sources.
#include <fstream>
#include <iterator>
#include <util/fs.h>

namespace fs_utils {
fs::path utf8_to_path(const std::string &value) { return fs::path(value); }
bool read_data(const fs::path &path, std::vector<char> &data) {
    std::ifstream file(path.string(), std::ios::binary);
    if (!file)
        return false;
    data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return !file.bad();
}
} // namespace fs_utils
