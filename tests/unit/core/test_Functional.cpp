#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <vector>

#include "core/Functional.hh"

using namespace jstine;

TEST(FunctionalTests, ConvertsRangesToVectors) {
    const std::array source{1, 2, 3};
    const auto out = source | toVector<int>();

    EXPECT_EQ(out, (std::vector<int>{1, 2, 3}));
}

TEST(FunctionalTests, GuardCallRunsOnDestruction) {
    int calls = 0;

    {
        GuardCall guard([&] { ++calls; });
        EXPECT_EQ(calls, 0);
    }

    EXPECT_EQ(calls, 1);
}

TEST(FunctionalTests, LazyEvaluationDefersExecution) {
    int calls = 0;
    auto lazy = LAZY_EVALUATE(++calls);

    EXPECT_EQ(calls, 0);
    EXPECT_EQ(static_cast<int>(lazy), 1);
    EXPECT_EQ(calls, 1);
}

TEST(FunctionalTests, MoveOnlyFunctionMovesAndInvokesMoveOnlyCallables) {
    auto value = std::make_unique<int>(41);
    MoveOnlyFunction<int(int)> function{
        [value = std::move(value)](int increment) { return *value + increment; }
    };

    EXPECT_FALSE(value);
    EXPECT_EQ(function(1), 42);

    auto moved = std::move(function);
    EXPECT_FALSE(function);
    EXPECT_TRUE(moved);
    EXPECT_EQ(moved(1), 42);
}

TEST(FunctionalTests, EmptyMoveOnlyFunctionThrows) {
    MoveOnlyFunction<void()> function;

    EXPECT_FALSE(function);
    EXPECT_THROW(function(), std::bad_function_call);
}
