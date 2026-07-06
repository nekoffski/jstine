#include <gtest/gtest.h>

#include <string>
#include <variant>

#include "api/Message.hh"

using namespace jstine;

TEST(MessageTests, ResponseOkPreservesPayload) {
    const Bytes payload{'o', 'k'};
    const Response response = Response::ok(payload);

    EXPECT_EQ(response.kind, ResponseKind::ok);
    ASSERT_TRUE(std::holds_alternative<OkResponseBody>(response.body));
    EXPECT_EQ(std::get<OkResponseBody>(response.body).payload, payload);
}

TEST(MessageTests, ResponseErrorFromCodeAndMessage) {
    const Response response = Response::error(ErrorCode::badInput, "bad input");

    EXPECT_EQ(response.kind, ResponseKind::error);
    ASSERT_TRUE(std::holds_alternative<ErrorResponseBody>(response.body));
    const auto& body = std::get<ErrorResponseBody>(response.body);
    EXPECT_EQ(body.code, static_cast<u32>(ErrorCode::badInput));
    EXPECT_EQ(body.message, Bytes({'b', 'a', 'd', ' ', 'i', 'n', 'p', 'u', 't'}));
}

TEST(MessageTests, ResponseErrorFromErrorObject) {
    const Error err{ErrorCode::notFound, "missing"};
    const Response response = Response::error(err);

    EXPECT_EQ(response.kind, ResponseKind::error);
    ASSERT_TRUE(std::holds_alternative<ErrorResponseBody>(response.body));
    const auto& body = std::get<ErrorResponseBody>(response.body);
    EXPECT_EQ(body.code, static_cast<u32>(ErrorCode::notFound));
    EXPECT_EQ(body.message, Bytes({'m', 'i', 's', 's', 'i', 'n', 'g'}));
}
