#include <cassert>
#include <ime/text.h>
#include <limits>

int main() {
    using ime::text_length;
    assert(text_length(u"", 0) == 0);
    assert(text_length(u"hello", 3) == 3);
    assert(text_length(u"hello", 0) == 0);
    assert(text_length(u"hello", std::numeric_limits<size_t>::max()) == 5);
    // Thai and Japanese remain UTF-16, without lossy byte conversion.
    assert(text_length(u"ภาษาไทย", 7) == 7);
    assert(text_length(u"日本語", 2) == 2);
    assert(text_length(u"A\U0001F600B", 2) == 1);
    assert(text_length(u"A\U0001F600B", 3) == 3);
    assert(text_length(u"\U0001F600", 1) == 0);
    assert(text_length(u"\U0001F600", 2) == 2);
    // Exercise every truncation point in a mixture of BMP and supplementary text.
    constexpr std::u16string_view sample = u"ไทย\U0001F600日本\U0001F680";
    for (size_t limit = 0; limit <= sample.size() + 2; ++limit) {
        const auto length = text_length(sample, limit);
        assert(length <= limit && length <= sample.size());
        if (length && length < sample.size())
            assert(!(sample[length - 1] >= 0xD800 && sample[length - 1] <= 0xDBFF));
    }
}
