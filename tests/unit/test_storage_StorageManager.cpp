#include <gtest/gtest.h>

#include "storage/StorageManager.hh"

using namespace jstine;

TEST(StorageManagerTests, StoresRetrievesAndRemovesValues) {
    StorageManager storage;
    const Key key{'k', 'e', 'y'};
    const Value value{'v', 'a', 'l'};

    EXPECT_FALSE(storage.exists(key));
    EXPECT_FALSE(storage.get(key));
    EXPECT_EQ(storage.get(key).error().code(), ErrorCode::notFound);

    EXPECT_FALSE(storage.set(key, value));
    EXPECT_TRUE(storage.exists(key));
    ASSERT_TRUE(storage.get(key));
    EXPECT_EQ(storage.get(key).value(), value);

    storage.remove(key);
    EXPECT_FALSE(storage.exists(key));
    EXPECT_FALSE(storage.get(key));
}
