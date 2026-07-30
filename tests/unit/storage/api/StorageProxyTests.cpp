#include <gtest/gtest.h>

#include "storage/StorageProxyTest.hh"
#include "storage/api/StorageProxy.hh"

namespace jstine {
namespace {

TEST_F(StorageProxyTest, ExecutesCommandsOnStorageExecutor) {
    StorageProxy storage{commandQueue};
    const Bytes key{'k', 'e', 'y'};
    const Bytes value{'v', 'a', 'l', 'u', 'e'};

    EXPECT_TRUE(storage.set(key, value));

    auto get = storage.get(key);
    EXPECT_TRUE(get);
    if (get) {
        EXPECT_EQ(*get, value);
    }

    auto exists = storage.exists(key);
    EXPECT_TRUE(exists);
    if (exists) {
        EXPECT_TRUE(*exists);
    }

    EXPECT_TRUE(storage.remove({key}));

    auto missing = storage.exists(key);
    EXPECT_TRUE(missing);
    if (missing) {
        EXPECT_FALSE(*missing);
    }
}

TEST_F(StorageProxyTest, ReportsUnavailableStorageAfterQueueClosure) {
    queue.close();
    executor.join();

    StorageProxy storage{commandQueue};
    const Bytes key{'k', 'e', 'y'};

    auto result = storage.exists(key);
    EXPECT_FALSE(result);
    if (not result) {
        EXPECT_EQ(result.error().code(), ErrorCode::storageUnavailable);
    }
}

}  // namespace
}  // namespace jstine
