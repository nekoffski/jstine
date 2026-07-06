#include <gtest/gtest.h>

#include <array>
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
