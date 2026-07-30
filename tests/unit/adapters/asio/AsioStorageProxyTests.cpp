#include <gtest/gtest.h>

#include <future>
#include <stdexcept>
#include <thread>

#include "adapters/asio/AsioStorageProxy.hh"
#include "storage/StorageProxyTest.hh"

namespace jstine {
namespace {

TEST_F(AsioStorageProxyTest, ExecutesCommandsOnStorageExecutor) {
    asio::io_context io;
    auto work = asio::make_work_guard(io);
    auto apiThread = std::this_thread::get_id();

    asio::co_spawn(
        io,
        [&]() -> asio::awaitable<void> {
            AsioStorageProxy storage{commandQueue};
            const Bytes key{'k', 'e', 'y'};
            const Bytes value{'v', 'a', 'l', 'u', 'e'};

            auto set = co_await storage.set(key, value);
            EXPECT_TRUE(set);

            auto get = co_await storage.get(key);
            EXPECT_TRUE(get);
            if (get) {
                EXPECT_EQ(*get, value);
            }

            auto exists = co_await storage.exists(key);
            EXPECT_TRUE(exists);
            if (exists) {
                EXPECT_TRUE(*exists);
            }

            auto remove = co_await storage.remove({key});
            EXPECT_TRUE(remove);

            auto missing = co_await storage.exists(key);
            EXPECT_TRUE(missing);
            if (missing) {
                EXPECT_FALSE(*missing);
            }
            EXPECT_EQ(std::this_thread::get_id(), apiThread);

            work.reset();
            co_return;
        },
        asio::detached
    );

    io.run();
}

TEST_F(AsioStorageProxyTest, ReportsUnavailableStorageAfterQueueClosure) {
    queue.close();

    asio::io_context io;
    auto work = asio::make_work_guard(io);

    asio::co_spawn(
        io,
        [&]() -> asio::awaitable<void> {
            AsioStorageProxy storage{commandQueue};
            const Bytes key{'k', 'e', 'y'};

            auto result = co_await storage.exists(key);
            EXPECT_FALSE(result);
            if (not result) {
                EXPECT_EQ(result.error().code(), ErrorCode::storageUnavailable);
            }

            work.reset();
            co_return;
        },
        asio::detached
    );

    io.run();
}

TEST_F(AsioStorageProxyTest, ExecutesTransactionOnStorageExecutor) {
    asio::io_context io;
    auto work = asio::make_work_guard(io);
    const auto apiThread = std::this_thread::get_id();

    asio::co_spawn(
        io,
        [&]() -> asio::awaitable<void> {
            AsioStorageProxy storage{commandQueue};
            const Bytes key{'k'};
            auto set = co_await storage.set(key, Bytes{'v'});
            EXPECT_TRUE(set);
            if (not set) {
                work.reset();
                co_return;
            }

            auto result = co_await storage.atomically(
                [&](StorageTransaction& transaction) {
                    EXPECT_NE(std::this_thread::get_id(), apiThread);
                    if (transaction.exists(key)) {
                        transaction.remove(key);
                        return true;
                    }
                    return false;
                }
            );

            EXPECT_TRUE(result);
            if (result) {
                EXPECT_TRUE(*result);
            }
            EXPECT_EQ(std::this_thread::get_id(), apiThread);

            auto exists = co_await storage.exists(key);
            EXPECT_TRUE(exists);
            if (exists) {
                EXPECT_FALSE(*exists);
            }

            work.reset();
        },
        asio::detached
    );

    io.run();
}

TEST_F(AsioStorageProxyTest, TransactionBodyCanReturnResult) {
    asio::io_context io;
    auto work = asio::make_work_guard(io);

    asio::co_spawn(
        io,
        [&]() -> asio::awaitable<void> {
            AsioStorageProxy storage{commandQueue};

            auto result = co_await storage.atomically(
                [](StorageTransaction& transaction) {
                    return transaction.get(Bytes{'m'});
                }
            );

            EXPECT_FALSE(result);
            if (not result) {
                EXPECT_EQ(result.error().code(), ErrorCode::notFound);
            }

            work.reset();
        },
        asio::detached
    );

    io.run();
}

TEST_F(AsioStorageProxyTest, TransactionReportsUnavailableStorage) {
    queue.close();

    asio::io_context io;
    auto work = asio::make_work_guard(io);
    bool called = false;

    asio::co_spawn(
        io,
        [&]() -> asio::awaitable<void> {
            AsioStorageProxy storage{commandQueue};

            auto result = co_await storage.atomically([&](StorageTransaction&) {
                called = true;
                return true;
            });

            EXPECT_FALSE(result);
            if (not result) {
                EXPECT_EQ(result.error().code(), ErrorCode::storageUnavailable);
            }
            EXPECT_FALSE(called);

            work.reset();
        },
        asio::detached
    );

    io.run();
}

TEST_F(AsioStorageProxyTest, TransactionPropagatesBodyException) {
    asio::io_context io;

    auto result = asio::co_spawn(
        io,
        [&]() -> asio::awaitable<void> {
            AsioStorageProxy storage{commandQueue};
            co_await storage.atomically([](StorageTransaction&) -> bool {
                throw std::runtime_error{"transaction failed"};
            });
        },
        asio::use_future
    );

    io.run();
    EXPECT_THROW(result.get(), std::runtime_error);
}

}  // namespace
}  // namespace jstine
