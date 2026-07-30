#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>

#include "storage/StorageProxyTest.hh"
#include "storage/api/StorageProxy.hh"

namespace jstine {
namespace {

using namespace std::chrono_literals;

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

TEST_F(StorageProxyTest, ExecutesTransactionAsOneQueuedCommand) {
    StorageProxy storage{commandQueue};
    const Bytes key{'k'};
    ASSERT_TRUE(storage.set(key, Bytes{'v'}));
    const auto callerThread = std::this_thread::get_id();

    auto result = storage.atomically([&](StorageTransaction& transaction) {
        EXPECT_NE(std::this_thread::get_id(), callerThread);
        if (transaction.exists(key)) {
            transaction.remove(key);
            return true;
        }
        return false;
    });

    ASSERT_TRUE(result);
    EXPECT_TRUE(*result);
    auto exists = storage.exists(key);
    ASSERT_TRUE(exists);
    EXPECT_FALSE(*exists);
}

TEST_F(StorageProxyTest, TransactionBodyCanReturnResult) {
    StorageProxy storage{commandQueue};

    auto result = storage.atomically([](StorageTransaction& transaction) {
        return transaction.get(Bytes{'m', 'i', 's', 's', 'i', 'n', 'g'});
    });

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::notFound);
}

TEST_F(StorageProxyTest, TransactionCanSetAndReadItsOwnWrite) {
    StorageProxy storage{commandQueue};
    const Bytes key{'k'};
    const Bytes value{'v'};

    auto result = storage.atomically(
        [&](StorageTransaction& transaction) -> Result<Bytes> {
            if (auto set = transaction.set(key, value); not set) {
                return std::unexpected(set.error());
            }
            return transaction.get(key);
        }
    );

    ASSERT_TRUE(result);
    EXPECT_EQ(*result, value);
}

TEST_F(StorageProxyTest, TransactionBodyCanReturnVoid) {
    StorageProxy storage{commandQueue};
    bool called = false;

    auto result =
        storage.atomically([&](StorageTransaction&) { called = true; });

    EXPECT_TRUE(result);
    EXPECT_TRUE(called);
}

TEST_F(StorageProxyTest, TransactionPreventsQueuedCommandInterleaving) {
    StorageProxy storage{commandQueue};
    const Bytes key{'k'};
    ASSERT_TRUE(storage.set(key, Bytes{'v'}));

    std::promise<void> entered;
    auto enteredFuture = entered.get_future();
    std::promise<void> release;
    auto releaseFuture = release.get_future().share();

    auto transactionFuture = std::async(std::launch::async, [&] {
        return storage.atomically([&](StorageTransaction& transaction) {
            entered.set_value();
            releaseFuture.wait();
            return transaction.get(key).has_value();
        });
    });

    enteredFuture.wait();

    std::promise<Result<void>> removal;
    auto removalFuture = removal.get_future();
    RemoveCommand command{};
    command.keyBytes = key;
    command.callback = [removal =
                            std::move(removal)](Result<void> result) mutable {
        removal.set_value(std::move(result));
    };

    const bool queued = commandQueue.push(Command{std::move(command)});
    EXPECT_TRUE(queued);
    if (queued) {
        EXPECT_EQ(removalFuture.wait_for(20ms), std::future_status::timeout);
    }

    release.set_value();

    auto transactionResult = transactionFuture.get();
    ASSERT_TRUE(transactionResult);
    EXPECT_TRUE(*transactionResult);
    if (queued) {
        EXPECT_TRUE(removalFuture.get());
    }
}

TEST_F(StorageProxyTest, TransactionReportsUnavailableStorage) {
    queue.close();
    executor.join();
    StorageProxy storage{commandQueue};
    bool called = false;

    auto result = storage.atomically([&](StorageTransaction&) {
        called = true;
        return true;
    });

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::storageUnavailable);
    EXPECT_FALSE(called);
}

TEST_F(StorageProxyTest, TransactionPropagatesBodyException) {
    StorageProxy storage{commandQueue};

    EXPECT_THROW(
        storage.atomically([](StorageTransaction&) -> bool {
            throw std::runtime_error{"transaction failed"};
        }),
        std::runtime_error
    );
}

}  // namespace
}  // namespace jstine
