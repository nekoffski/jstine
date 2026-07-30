#include <gtest/gtest.h>

#include <chrono>

#include "core/Config.hh"
#include "storage/Database.hh"
#include "storage/ExpirationRegistry.hh"
#include "storage/keyspace/StdKeyspace.hh"

using namespace jstine;
using namespace std::chrono_literals;

namespace {

struct DatabaseFixture {
    explicit DatabaseFixture(std::chrono::seconds expiration) {
        config.storage().defaultExpiration = expiration;
    }

    Config config;
    StdKeyspace keyspace;
    ExpirationRegistry expirationRegistry;
    Database database{config, keyspace, expirationRegistry};
};

}  // namespace

TEST(DatabaseTests, MissingGetReturnsNotFound) {
    DatabaseFixture fixture{60s};

    const auto result = fixture.database.get(Bytes{'k'});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::notFound);
    EXPECT_EQ(result.error().message(), "Key does not exist");
}

TEST(DatabaseTests, ExpiredKeyIsRemovedOnGet) {
    DatabaseFixture fixture{-1s};
    const Bytes key{'k'};
    ASSERT_FALSE(fixture.database.set(key, Bytes{'v'}));

    const auto result = fixture.database.get(key);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::notFound);
    EXPECT_EQ(result.error().message(), "Key has expired");
    EXPECT_FALSE(fixture.keyspace.exists(Key{key}));
}

TEST(DatabaseTests, ExpiredKeyDoesNotExistAndIsRemoved) {
    DatabaseFixture fixture{-1s};
    const Bytes key{'k'};
    ASSERT_FALSE(fixture.database.set(key, Bytes{'v'}));

    EXPECT_FALSE(fixture.database.exists(key));
    EXPECT_FALSE(fixture.keyspace.exists(Key{key}));
}
