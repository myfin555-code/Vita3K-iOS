#pragma once
#include <algorithm>
#include <cstddef>
#include <string_view>

namespace ime {
// Vita limits count UTF-16 code units. Never cut a surrogate pair in half.
inline std::size_t text_length(std::u16string_view text, std::size_t limit) {
    std::size_t length = std::min(text.size(), limit);
    if (length && length < text.size() && text[length - 1] >= 0xD800
        && text[length - 1] <= 0xDBFF && text[length] >= 0xDC00 && text[length] <= 0xDFFF)
        --length;
    return length;
}
} // namespace ime
