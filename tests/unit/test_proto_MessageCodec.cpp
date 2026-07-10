#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <span>
#include <variant>

#include "proto/MessageCodec.hh"
#include "proto/impl/JFP.hh"

using namespace jstine;

TEST(MessageCodecTests, ConstructsJfpCodecWithMatchingProtocol) {
    MessageCodec codec{Protocol::jfp};

    EXPECT_EQ(codec.protocol(), Protocol::jfp);
    EXPECT_EQ(protocolToStr(codec.protocol()), "jfp");
}

TEST(MessageCodecTests, JfpCodecCanDecodeAndEncode) {
    MessageCodec codec{Protocol::jfp};

    const Bytes key{'k', 'e', 'y'};
    const Bytes value{'v', 'a', 'l'};

    Bytes requestFrame;
    requestFrame.resize(8);
    const u32 payloadSize = 4 + 5 + static_cast<u32>(key.size()) + 5 +
                            static_cast<u32>(value.size());
    std::memcpy(requestFrame.data(), &payloadSize, sizeof(payloadSize));
    const u32 kind = static_cast<u32>(RequestKind::set);
    std::memcpy(requestFrame.data() + 4, &kind, sizeof(kind));

    const auto appendField = [&](JFPFieldType type, const Bytes& data) {
        requestFrame.push_back(static_cast<u8>(type));
        const u32 size = static_cast<u32>(data.size());
        const auto* sizeBytes = reinterpret_cast<const Byte*>(&size);
        requestFrame.insert(
            requestFrame.end(), sizeBytes, sizeBytes + sizeof(size)
        );
        requestFrame.insert(requestFrame.end(), data.begin(), data.end());
    };
    appendField(JFPFieldType::key, key);
    appendField(JFPFieldType::value, value);

    codec.feed(requestFrame);
    const auto decoded = codec.decode();
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->kind, RequestKind::set);
    ASSERT_TRUE(std::holds_alternative<SetRequestBody>(decoded->body));
    const auto& body = std::get<SetRequestBody>(decoded->body);
    EXPECT_EQ(body.key, key);
    EXPECT_EQ(body.value, value);

    const Response response = Response::ok(Bytes{'o', 'k'});
    std::array<Byte, 64> out{};
    const auto encoded = codec.encode(response, out);
    ASSERT_TRUE(encoded);
    EXPECT_EQ(*encoded, 15u);
    EXPECT_EQ(
        Bytes(out.begin(), out.begin() + static_cast<ptrdiff_t>(*encoded)),
        (Bytes{11, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 'o', 'k'})
    );
}
