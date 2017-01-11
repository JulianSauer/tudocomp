
#include "tudocomp/tudocomp.hpp"
#include "test/util.hpp"
#include <gtest/gtest.h>

struct InputOutput {
    View in;
    View out;
};

std::ostream& operator<<(std::ostream& os, const InputOutput& v) {
    return os << v.in << " : " << v.out;
}

using namespace tdc;

TEST(Lz78U, test) {
    auto c = create_algo<LZ78UCompressor<ASCIICoder, lz78u::AsciiNt>>();

    //auto i = test::compress_input("asd\0asdfgsdf"_v);
    auto i = test::compress_input("aaababaaabaababa"_v);
    auto o = test::compress_output();

    c.compress(i, o);

    ASSERT_EQ(o.result(), "0:a\0""1:a\0""0:ba\0""3:a\0""1:ba\0""5:ba\0""\0"_v);

}

TEST(Lz78U, roundtrip1) {
    test::roundtrip<LZ78UCompressor<ASCIICoder, lz78u::AsciiNt>>(
        "aaababaaabaababa"_v,
        "0:a\0""1:a\0""0:ba\0""3:a\0""1:ba\0""5:ba\0""\0"_v
    );
}

TEST(Lz78U, roundtrip2) {
    test::roundtrip<LZ78UCompressor<ASCIICoder, lz78u::AsciiNt>>(
        "abcdebcdeabc"_v,
        ""_v
    );
}
