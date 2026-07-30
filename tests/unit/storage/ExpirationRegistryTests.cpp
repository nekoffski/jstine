#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include "storage/ExpirationRegistry.hh"

using namespace jstine;
using namespace std::chrono_literals;

TEST(ExpirationRegistryTests, MissingKeyIsNotExpired) {
    ExpirationRegistry registry;

    EXPECT_FALSE(registry.expired(Key{Bytes{'k'}}));
}

TEST(ExpirationRegistryTests, ReportsAndRemovesExpiredKey) {
    ExpirationRegistry registry;
    const Key key{Bytes{'k'}};
    registry.expiresAfter(key, 1s);

    EXPECT_TRUE(registry.expired(key, Clock::now() + 2s));
    EXPECT_FALSE(registry.expired(key, Clock::now() + 2s));
}

TEST(ExpirationRegistryTests, DoesNotExpireKeyBeforeDeadline) {
    ExpirationRegistry registry;
    const Key key{Bytes{'k'}};
    registry.expiresAfter(key, 2s);

    EXPECT_FALSE(registry.expired(key, Clock::now() + 1s));
}

TEST(ExpirationRegistryTests, ForEachStopsWhenCallbackReturnsFalse) {
    ExpirationRegistry registry;
    registry.expiresAfter(Key{Bytes{'a'}}, 1s);
    registry.expiresAfter(Key{Bytes{'b'}}, 2s);
    u64 visits = 0;

    registry.forEach([&](const Key&, const Clock::time_point&) {
        ++visits;
        return false;
    });

    EXPECT_EQ(visits, 1u);
}
