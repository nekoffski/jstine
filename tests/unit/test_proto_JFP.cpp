#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <span>
#include <utility>

#include "proto/impl/JFP.hh"

using namespace jstine;

namespace {

void appendField(Bytes& frame, JFPFieldType type, std::span<const Byte> data) {
    frame.push_back(static_cast<u8>(type));
    const u32 size = static_cast<u32>(data.size());
    const auto* sizeBytes = reinterpret_cast<const Byte*>(&size);
    frame.insert(frame.end(), sizeBytes, sizeBytes + sizeof(size));
    frame.insert(frame.end(), data.begin(), data.end());
}

Bytes makeRequestFrame(
    RequestKind kind,
    std::initializer_list<std::pair<JFPFieldType, Bytes>> fields
) {
    Bytes frame(8);
    const auto fieldBytes = [&]() {
        Bytes out;
        for (const auto& [type, data] : fields) {
            appendField(out, type, data);
        }
        return out;
    }();

    const u32 payloadSize = 4 + static_cast<u32>(fieldBytes.size());
    const u32 kindValue = static_cast<u32>(kind);
    std::memcpy(frame.data(), &payloadSize, sizeof(payloadSize));
    std::memcpy(frame.data() + 4, &kindValue, sizeof(kindValue));
    frame.insert(frame.end(), fieldBytes.begin(), fieldBytes.end());
    return frame;
}

}  // namespace

TEST(JFPTests, DecoderReportsRequestNotReadyForEmptyBuffer) {
    JFPRequestDecoder decoder;

    const auto decoded = decoder.decode();

    ASSERT_FALSE(decoded);
    EXPECT_EQ(decoded.error().code(), ErrorCode::requestNotReady);
}

TEST(JFPTests, DecoderReportsRequestNotReadyForPartialHeader) {
    JFPRequestDecoder decoder;
    decoder.feed(Bytes{1, 2, 3, 4, 5, 6, 7});

    const auto decoded = decoder.decode();

    ASSERT_FALSE(decoded);
    EXPECT_EQ(decoded.error().code(), ErrorCode::requestNotReady);
}

TEST(JFPTests, DecoderParsesPingWithoutPayload) {
    JFPRequestDecoder decoder;
    decoder.feed(makeRequestFrame(RequestKind::ping, {}));

    const auto decoded = decoder.decode();

    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->kind, RequestKind::ping);
    ASSERT_TRUE(std::holds_alternative<PingRequestBody>(decoded->body));
    EXPECT_TRUE(std::get<PingRequestBody>(decoded->body).payload.empty());
}

TEST(JFPTests, DecoderParsesPingWithPayload) {
    JFPRequestDecoder decoder;
    const Bytes payload{'h', 'e', 'l', 'l', 'o'};
    decoder.feed(
        makeRequestFrame(RequestKind::ping, {{JFPFieldType::payload, payload}})
    );

    const auto decoded = decoder.decode();

    ASSERT_TRUE(decoded);
    ASSERT_TRUE(std::holds_alternative<PingRequestBody>(decoded->body));
    EXPECT_EQ(std::get<PingRequestBody>(decoded->body).payload, payload);
}

TEST(JFPTests, DecoderUsesFirstMatchingField) {
    JFPRequestDecoder decoder;
    decoder.feed(makeRequestFrame(
        RequestKind::ping,
        {
            {JFPFieldType::payload, Bytes{'f', 'i', 'r', 's', 't'}},
            {JFPFieldType::payload, Bytes{'s', 'e', 'c', 'o', 'n', 'd'}},
        }
    ));

    const auto decoded = decoder.decode();

    ASSERT_TRUE(decoded);
    ASSERT_TRUE(std::holds_alternative<PingRequestBody>(decoded->body));
    EXPECT_EQ(
        std::get<PingRequestBody>(decoded->body).payload,
        (Bytes{'f', 'i', 'r', 's', 't'})
    );
}

TEST(JFPTests, DecoderParsesSetRequest) {
    JFPRequestDecoder decoder;
    const Bytes key{'k', 'e', 'y'};
    const Bytes value{'v', 'a', 'l'};
    decoder.feed(makeRequestFrame(
        RequestKind::set,
        {
            {JFPFieldType::key, key},
            {JFPFieldType::value, value},
        }
    ));

    const auto decoded = decoder.decode();

    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->kind, RequestKind::set);
    ASSERT_TRUE(std::holds_alternative<SetRequestBody>(decoded->body));
    const auto& body = std::get<SetRequestBody>(decoded->body);
    EXPECT_EQ(body.key, key);
    EXPECT_EQ(body.value, value);
}

TEST(JFPTests, DecoderParsesGetDeleteAndExistsRequests) {
    const Bytes key{'k', 'e', 'y'};

    {
        JFPRequestDecoder decoder;
        decoder.feed(
            makeRequestFrame(RequestKind::get, {{JFPFieldType::key, key}})
        );

        const auto decoded = decoder.decode();
        ASSERT_TRUE(decoded);
        EXPECT_EQ(decoded->kind, RequestKind::get);
        ASSERT_TRUE(std::holds_alternative<GetRequestBody>(decoded->body));
        EXPECT_EQ(std::get<GetRequestBody>(decoded->body).key, key);
    }

    {
        JFPRequestDecoder decoder;
        decoder.feed(
            makeRequestFrame(RequestKind::del, {{JFPFieldType::key, key}})
        );

        const auto decoded = decoder.decode();
        ASSERT_TRUE(decoded);
        EXPECT_EQ(decoded->kind, RequestKind::del);
        ASSERT_TRUE(std::holds_alternative<DelRequestBody>(decoded->body));
        EXPECT_EQ(std::get<DelRequestBody>(decoded->body).key, key);
    }

    {
        JFPRequestDecoder decoder;
        decoder.feed(
            makeRequestFrame(RequestKind::exists, {{JFPFieldType::key, key}})
        );

        const auto decoded = decoder.decode();
        ASSERT_TRUE(decoded);
        EXPECT_EQ(decoded->kind, RequestKind::exists);
        ASSERT_TRUE(std::holds_alternative<ExistsRequestBody>(decoded->body));
        EXPECT_EQ(std::get<ExistsRequestBody>(decoded->body).key, key);
    }
}

TEST(JFPTests, DecoderReportsMissingFieldsForRequests) {
    {
        JFPRequestDecoder decoder;
        decoder.feed(makeRequestFrame(RequestKind::get, {}));

        const auto decoded = decoder.decode();
        ASSERT_FALSE(decoded);
        EXPECT_EQ(decoded.error().code(), ErrorCode::badInput);
        EXPECT_EQ(decoded.error().message(), "get: missing key");
    }

    {
        JFPRequestDecoder decoder;
        decoder.feed(makeRequestFrame(
            RequestKind::set, {{JFPFieldType::key, Bytes{'k'}}}
        ));

        const auto decoded = decoder.decode();
        ASSERT_FALSE(decoded);
        EXPECT_EQ(decoded.error().code(), ErrorCode::badInput);
        EXPECT_EQ(decoded.error().message(), "set: missing value");
    }

    {
        JFPRequestDecoder decoder;
        decoder.feed(makeRequestFrame(RequestKind::del, {}));

        const auto decoded = decoder.decode();
        ASSERT_FALSE(decoded);
        EXPECT_EQ(decoded.error().code(), ErrorCode::badInput);
        EXPECT_EQ(decoded.error().message(), "del: missing key");
    }

    {
        JFPRequestDecoder decoder;
        decoder.feed(makeRequestFrame(RequestKind::exists, {}));

        const auto decoded = decoder.decode();
        ASSERT_FALSE(decoded);
        EXPECT_EQ(decoded.error().code(), ErrorCode::badInput);
        EXPECT_EQ(decoded.error().message(), "exists: missing key");
    }
}

TEST(JFPTests, DecoderReportsUnknownRequestKind) {
    JFPRequestDecoder decoder;
    decoder.feed(makeRequestFrame(static_cast<RequestKind>(999), {}));

    const auto decoded = decoder.decode();

    ASSERT_FALSE(decoded);
    EXPECT_EQ(decoded.error().code(), ErrorCode::badInput);
    EXPECT_EQ(decoded.error().message(), "Unknown request kind: 999");
}

TEST(JFPTests, DecoderReportsTruncatedFieldHeader) {
    JFPRequestDecoder decoder;
    Bytes frame(12);
    const u32 payloadSize = 8;
    const u32 kind = static_cast<u32>(RequestKind::ping);
    std::memcpy(frame.data(), &payloadSize, sizeof(payloadSize));
    std::memcpy(frame.data() + 4, &kind, sizeof(kind));
    frame[8] = static_cast<Byte>(JFPFieldType::payload);

    decoder.feed(frame);

    const auto decoded = decoder.decode();

    ASSERT_FALSE(decoded);
    EXPECT_EQ(decoded.error().code(), ErrorCode::badInput);
    EXPECT_EQ(decoded.error().message(), "Truncated field header");
}

TEST(JFPTests, DecoderReportsFieldDataOutOfBounds) {
    JFPRequestDecoder decoder;
    Bytes frame(16);
    const u32 payloadSize = 12;
    const u32 kind = static_cast<u32>(RequestKind::set);
    std::memcpy(frame.data(), &payloadSize, sizeof(payloadSize));
    std::memcpy(frame.data() + 4, &kind, sizeof(kind));
    frame[8] = static_cast<Byte>(JFPFieldType::key);
    const u32 fieldSize = 8;
    std::memcpy(frame.data() + 9, &fieldSize, sizeof(fieldSize));
    frame[13] = 'a';
    frame[14] = 'b';
    frame[15] = 'c';

    decoder.feed(frame);

    const auto decoded = decoder.decode();

    ASSERT_FALSE(decoded);
    EXPECT_EQ(decoded.error().code(), ErrorCode::badInput);
    EXPECT_EQ(decoded.error().message(), "Field data out of bounds");
}

TEST(JFPTests, DecoderConsumesOneFrameAndKeepsTrailingBytes) {
    JFPRequestDecoder decoder;
    const auto first = makeRequestFrame(
        RequestKind::set,
        {
            {JFPFieldType::key, Bytes{'a'}},
            {JFPFieldType::value, Bytes{'1'}},
        }
    );
    const auto second =
        makeRequestFrame(RequestKind::get, {{JFPFieldType::key, Bytes{'a'}}});

    Bytes combined = first;
    combined.insert(combined.end(), second.begin(), second.end());
    decoder.feed(combined);

    const auto firstDecoded = decoder.decode();
    ASSERT_TRUE(firstDecoded);
    EXPECT_EQ(firstDecoded->kind, RequestKind::set);

    const auto secondDecoded = decoder.decode();
    ASSERT_TRUE(secondDecoded);
    EXPECT_EQ(secondDecoded->kind, RequestKind::get);
}

TEST(JFPTests, DecoderSupportsIncrementalFeedAcrossCalls) {
    JFPRequestDecoder decoder;
    const auto frame = makeRequestFrame(
        RequestKind::set,
        {
            {JFPFieldType::key, Bytes{'k', 'e', 'y'}},
            {JFPFieldType::value, Bytes{'v', 'a', 'l'}},
        }
    );

    decoder.feed({frame.data(), 3});
    auto decoded = decoder.decode();
    ASSERT_FALSE(decoded);
    EXPECT_EQ(decoded.error().code(), ErrorCode::requestNotReady);

    decoder.feed({frame.data() + 3, frame.size() - 3});
    decoded = decoder.decode();
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->kind, RequestKind::set);
}

TEST(JFPTests, EncoderSerializesOkResponseWithPayload) {
    JFPResponseEncoder encoder;
    const Response response = Response::ok(Bytes{'o', 'k'});
    std::array<Byte, 64> out{};

    const auto encoded = encoder.encode(response, out);

    ASSERT_TRUE(encoded);
    EXPECT_EQ(*encoded, 15u);
    EXPECT_EQ(
        Bytes(out.begin(), out.begin() + static_cast<ptrdiff_t>(*encoded)),
        (Bytes{11, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 'o', 'k'})
    );
}

TEST(JFPTests, EncoderSerializesOkResponseWithoutPayload) {
    JFPResponseEncoder encoder;
    const Response response = Response::ok();
    std::array<Byte, 64> out{};

    const auto encoded = encoder.encode(response, out);

    ASSERT_TRUE(encoded);
    EXPECT_EQ(*encoded, 8u);
    EXPECT_EQ(
        Bytes(out.begin(), out.begin() + static_cast<ptrdiff_t>(*encoded)),
        (Bytes{4, 0, 0, 0, 0, 0, 0, 0})
    );
}

TEST(JFPTests, EncoderSerializesErrorResponseWithCodeOnly) {
    JFPResponseEncoder encoder;
    const Response response = Response::error(ErrorCode::notFound, "");
    std::array<Byte, 64> out{};

    const auto encoded = encoder.encode(response, out);

    ASSERT_TRUE(encoded);
    EXPECT_EQ(*encoded, 17u);
    EXPECT_EQ(
        Bytes(out.begin(), out.begin() + static_cast<ptrdiff_t>(*encoded)),
        (Bytes{13, 0, 0, 0, 1, 0, 0, 0, 4, 4, 0, 0, 0, 11, 0, 0, 0})
    );
}

TEST(JFPTests, EncoderSerializesErrorResponseWithCodeAndMessage) {
    JFPResponseEncoder encoder;
    const Response response = Response::error(ErrorCode::notFound, "missing");
    std::array<Byte, 64> out{};

    const auto encoded = encoder.encode(response, out);

    ASSERT_TRUE(encoded);
    EXPECT_EQ(*encoded, 29u);
    EXPECT_EQ(
        Bytes(out.begin(), out.begin() + static_cast<ptrdiff_t>(*encoded)),
        (Bytes{25, 0, 0, 0, 1, 0, 0, 0,   4,   4,   0,   0,   0,   11, 0,
               0,  0, 5, 7, 0, 0, 0, 'm', 'i', 's', 's', 'i', 'n', 'g'})
    );
}

TEST(JFPTests, EncoderRejectsTooSmallBuffer) {
    JFPResponseEncoder encoder;
    const Response response = Response::ok(Bytes{'o', 'k'});
    std::array<Byte, 4> out{};

    const auto encoded = encoder.encode(response, out);

    ASSERT_FALSE(encoded);
    EXPECT_EQ(encoded.error().code(), ErrorCode::badInput);
    EXPECT_EQ(
        encoded.error().message(), "Output buffer too small: need 15, have 4"
    );
}
