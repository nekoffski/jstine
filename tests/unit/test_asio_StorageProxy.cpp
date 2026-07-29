#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "adapters/asio/AsioStorageProxy.hh"
#include "core/Config.hh"
#include "storage/Database.hh"
#include "storage/ExpirationRegistry.hh"
#include "storage/api/StorageCommandQueue.hh"
#include "storage/api/StorageExecutor.hh"
#include "storage/keyspace/StdKeyspace.hh"

namespace jstine {
namespace {

TEST(AsioStorageProxy, ExecutesCommandsOnStorageExecutor) {
    Config config;
    config.storage().defaultExpiration = std::chrono::seconds{60};

    StdKeyspace keyspace;
    ExpirationRegistry expirationRegistry;
    Database database{config, keyspace, expirationRegistry};
    ThreadSafeQueue<Command> queue;
    StorageCommandQueue commandQueue{queue};
    StorageExecutor executor{database, queue};
    executor.start();

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

            auto remove = co_await storage.remove(key);
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
    executor.cancel();
    executor.join();
}

TEST(AsioStorageProxy, ReportsUnavailableStorageAfterQueueClosure) {
    ThreadSafeQueue<Command> queue;
    StorageCommandQueue commandQueue{queue};
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

}  // namespace
}  // namespace jstine
