#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <span>

#include "proto/impl/JFP.hh"

using namespace jstine;

static Bytes makeFrame(
    u32 kind, std::span<const Byte> first, std::span<const Byte> second = {}
) {
    Bytes frame(8);
    const u32 payloadSize = 4 + 5 + static_cast<u32>(first.size()) +
                            (second.empty() ? 0u
                                            : 5u + static_cast<u32>(second.size()));
    std::memcpy(frame.data(), &payloadSize, sizeof(payloadSize));
    std::memcpy(frame.data() + 4, &kind, sizeof(kind));

    const auto appendField = [&](JFPFieldType type,
                                 std::span<const Byte> data) {
        frame.push_back(static_cast<u8>(type));
        const u32 size = static_cast<u32>(data.size());
        const auto* sizeBytes = reinterpret_cast<const Byte*>(&size);
        frame.insert(frame.end(), sizeBytes, sizeBytes + sizeof(size));
        frame.insert(frame.end(), data.begin(), data.end());
    };

    appendField(JFPFieldType::key, first);
    if (not second.empty()) {
        appendField(JFPFieldType::value, second);
    }

    return frame;
}

TEST(JFPTests, DecoderParsesSetRequest) {
    JFPRequestDecoder decoder;
    const Bytes key{'k', 'e', 'y'};
    const Bytes value{'v', 'a', 'l'};
    const auto frame =
        makeFrame(static_cast<u32>(RequestKind::set), key, value);

    decoder.feed(frame);
    const auto decoded = decoder.decode();

    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->kind, RequestKind::set);
    ASSERT_TRUE(std::holds_alternative<SetRequestBody>(decoded->body));
    const auto& body = std::get<SetRequestBody>(decoded->body);
    EXPECT_EQ(body.key, key);
    EXPECT_EQ(body.value, value);
}

TEST(JFPTests, DecoderReportsIncompleteFrame) {
    JFPRequestDecoder decoder;
    const Bytes partial{1, 2, 3, 4, 5, 6, 7};

    decoder.feed(partial);
    const auto decoded = decoder.decode();

    ASSERT_FALSE(decoded);
    EXPECT_EQ(decoded.error().code(), ErrorCode::requestNotReady);
}

TEST(JFPTests, EncoderSerializesErrorResponse) {
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
