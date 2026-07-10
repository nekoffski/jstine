#include <gtest/gtest.h>

#include "core/String.hh"

using namespace jstine;

TEST(StringTests, SplitsStringsOnSeparator) {
    EXPECT_EQ(split("a::b::", "::"), (std::vector<std::string>{"a", "b", ""}));
}

TEST(StringTests, ExtractsNamesAndExtensions) {
    EXPECT_EQ(
        nameFromPath("/tmp/archive.tar.gz", NameExtractionMode::withExtension),
        "archive.tar.gz"
    );
    EXPECT_EQ(
        nameFromPath(
            "/tmp/archive.tar.gz", NameExtractionMode::withoutFullExtension
        ),
        "archive"
    );
    EXPECT_EQ(
        nameFromPath(
            "/tmp/archive.tar.gz", NameExtractionMode::withoutLastExtensionChunk
        ),
        "archive.tar"
    );

    EXPECT_EQ(
        extensionFromPath("/tmp/archive.tar.gz", ExtensionExtractionMode::full),
        ".tar.gz"
    );
    EXPECT_EQ(
        extensionFromPath(
            "/tmp/archive.tar.gz", ExtensionExtractionMode::lastChunk
        ),
        ".gz"
    );
    EXPECT_FALSE(extensionFromPath("/tmp/archive").has_value());
}

TEST(StringTests, FormatsBytesAsBinaryHexAndHexDump) {
    const Bytes bytes{0x00, 0x41, 0xff};

    EXPECT_EQ(toBinaryString(bytes), "000000000100000111111111");
    EXPECT_EQ(toHexString(bytes), "0041ff");
    EXPECT_EQ(hexDump(bytes), "00 41 ff | .A.");
}
