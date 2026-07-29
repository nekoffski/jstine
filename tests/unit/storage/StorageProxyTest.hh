#pragma once

#include <gtest/gtest.h>

#include <chrono>

#include "core/Config.hh"
#include "storage/Database.hh"
#include "storage/ExpirationRegistry.hh"
#include "storage/api/StorageCommandQueue.hh"
#include "storage/api/StorageExecutor.hh"
#include "storage/keyspace/StdKeyspace.hh"

namespace jstine {

class StorageProxyTest : public ::testing::Test {
   protected:
    StorageProxyTest() {
        config.storage().defaultExpiration = std::chrono::seconds{60};
    }

    void SetUp() override { executor.start(); }

    void TearDown() override {
        executor.cancel();
        executor.join();
    }

    Config config;
    StdKeyspace keyspace;
    ExpirationRegistry expirationRegistry;
    Database database{config, keyspace, expirationRegistry};
    ThreadSafeQueue<Command> queue;
    StorageCommandQueue commandQueue{queue};
    StorageExecutor executor{database, queue};
};

class AsioStorageProxyTest : public StorageProxyTest {};

}  // namespace jstine
