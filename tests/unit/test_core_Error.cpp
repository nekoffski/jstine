#include <gtest/gtest.h>

#include "core/Error.hh"

using namespace jstine;

TEST(ErrorTests, StoresCodeAndMessage) {
    const Error error{ErrorCode::badInput, "bad request"};

    EXPECT_EQ(error.code(), ErrorCode::badInput);
    EXPECT_EQ(error.message(), "bad request");
}

TEST(ErrorTests, UnexpectedWrapsError) {
    const auto unexpected = Error::unexpected(ErrorCode::notFound, "missing {}", 3);

    EXPECT_EQ(unexpected.error().code(), ErrorCode::notFound);
    EXPECT_EQ(unexpected.error().message(), "missing 3");
}

TEST(ErrorTests, EmptyProducesNoValue) {
    EXPECT_FALSE(Error::empty().has_value());
}
