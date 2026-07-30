#include <gtest/gtest.h>

#include "storage/kv/Key.hh"

using namespace jstine;

TEST(KeyTests, ConstructsFromCopiedMovedAndViewedBytes) {
    const Bytes copiedBytes{'c', 'o', 'p', 'y'};
    Bytes movedBytes{'m', 'o', 'v', 'e'};
    const Bytes viewedBytes{'v', 'i', 'e', 'w'};

    const Key copied{copiedBytes};
    const Key moved{std::move(movedBytes)};
    const Key viewed{CBytesView{viewedBytes}};

    EXPECT_EQ(copied.bytes(), copiedBytes);
    EXPECT_EQ(moved.bytes(), (Bytes{'m', 'o', 'v', 'e'}));
    EXPECT_EQ(viewed.bytes(), viewedBytes);
}

TEST(KeyTests, EqualityComparesBytes) {
    const Key first{Bytes{'k'}};
    const Key same{Bytes{'k'}};
    const Key different{Bytes{'x'}};

    EXPECT_EQ(first, same);
    EXPECT_NE(first, different);
}
