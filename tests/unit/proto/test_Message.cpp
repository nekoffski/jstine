#include <gtest/gtest.h>

#include <string>
#include <variant>

#include "proto/Message.hh"

using namespace jstine;

TEST(MessageTests, ResponseOkPreservesPayload) {
    const Bytes payload{'o', 'k'};
    const Response response = Response::ok(payload);

    EXPECT_EQ(response.kind, ResponseKind::ok);
    ASSERT_TRUE(std::holds_alternative<OkResponseBody>(response.body));
    EXPECT_EQ(std::get<OkResponseBody>(response.body).payload, payload);
}

TEST(MessageTests, ResponseOkDefaultsToEmptyPayload) {
    const Response response = Response::ok();

    EXPECT_EQ(response.kind, ResponseKind::ok);
    ASSERT_TRUE(std::holds_alternative<OkResponseBody>(response.body));
    EXPECT_TRUE(std::get<OkResponseBody>(response.body).payload.empty());
}

TEST(MessageTests, ResponseErrorFromCodeAndMessage) {
    const Response response = Response::error(ErrorCode::badInput, "bad input");

    EXPECT_EQ(response.kind, ResponseKind::error);
    ASSERT_TRUE(std::holds_alternative<ErrorResponseBody>(response.body));
    const auto& body = std::get<ErrorResponseBody>(response.body);
    EXPECT_EQ(body.code, static_cast<u32>(ErrorCode::badInput));
    EXPECT_EQ(
        body.message, Bytes({'b', 'a', 'd', ' ', 'i', 'n', 'p', 'u', 't'})
    );
}

TEST(MessageTests, ResponseErrorFromCodeCanCarryEmptyMessage) {
    const Response response = Response::error(ErrorCode::badInput, "");

    EXPECT_EQ(response.kind, ResponseKind::error);
    ASSERT_TRUE(std::holds_alternative<ErrorResponseBody>(response.body));
    const auto& body = std::get<ErrorResponseBody>(response.body);
    EXPECT_EQ(body.code, static_cast<u32>(ErrorCode::badInput));
    EXPECT_TRUE(body.message.empty());
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

TEST(MessageTests, SetRequestDefaultsToNoCondition) {
    const SetRequestBody body{Bytes{'k'}, Bytes{'v'}};

    EXPECT_EQ(body.condition, SetCondition::none);
}

TEST(MessageTests, ModelsExpirationRequests) {
    const Request ttl{RequestKind::ttl, TtlRequestBody{Bytes{'k'}}};
    const Request persist{RequestKind::persist, PersistRequestBody{Bytes{'k'}}};
    const Request expire{
        RequestKind::expire, ExpireRequestBody{Bytes{'k'}, 42}
    };

    EXPECT_TRUE(std::holds_alternative<TtlRequestBody>(ttl.body));
    EXPECT_TRUE(std::holds_alternative<PersistRequestBody>(persist.body));
    ASSERT_TRUE(std::holds_alternative<ExpireRequestBody>(expire.body));
    EXPECT_EQ(std::get<ExpireRequestBody>(expire.body).seconds, 42u);
}
