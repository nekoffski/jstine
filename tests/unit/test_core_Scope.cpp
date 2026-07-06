#include <gtest/gtest.h>

#include <utility>

#include "core/Scope.hh"

using namespace jstine;

TEST(ScopeTests, ScopedCallsDestructor) {
    int observed = 0;

    {
        details::Scoped<int> scoped{42, [&](int& value) { observed = value; }};
        EXPECT_FALSE(scoped.empty());
        EXPECT_EQ(*scoped, 42);
    }

    EXPECT_EQ(observed, 42);
}

TEST(ScopeTests, ScopedMoveTransfersOwnership) {
    int calls = 0;

    {
        details::Scoped<int> source{7, [&](int&) { ++calls; }};
        {
            details::Scoped<int> moved{std::move(source)};

            EXPECT_TRUE(source.empty());
            EXPECT_FALSE(moved.empty());
            EXPECT_EQ(moved.get(), 7);
        }
        EXPECT_EQ(calls, 1);
    }
}
