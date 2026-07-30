#include <gtest/gtest.h>

#include "storage/keyspace/StdKeyspace.hh"

using namespace jstine;

TEST(StdKeyspaceTests, ReservesAndReturnsSameValueForExistingKey) {
    StdKeyspace keyspace;
    const Key key{Bytes{'k'}};

    EXPECT_FALSE(keyspace.exists(key));
    EXPECT_EQ(keyspace.get(key), nullptr);

    auto first = keyspace.reserve(key);
    ASSERT_TRUE(first);
    EXPECT_TRUE(keyspace.exists(key));
    EXPECT_EQ(keyspace.get(key), &first->get());

    auto second = keyspace.reserve(key);
    ASSERT_TRUE(second);
    EXPECT_EQ(&second->get(), &first->get());
}

TEST(StdKeyspaceTests, RemovesReservedKey) {
    StdKeyspace keyspace;
    const Key key{Bytes{'k'}};
    ASSERT_TRUE(keyspace.reserve(key));

    keyspace.remove(key);

    EXPECT_FALSE(keyspace.exists(key));
    EXPECT_EQ(keyspace.get(key), nullptr);
}
