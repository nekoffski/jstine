#include <gtest/gtest.h>

#include "proto/Protocol.hh"

using namespace jstine;

TEST(ProtocolTests, ProtocolToStrMapsKnownProtocols) {
    EXPECT_EQ(protocolToStr(Protocol::rsp), "rsp");
    EXPECT_EQ(protocolToStr(Protocol::jfp), "jfp");
    EXPECT_EQ(protocolToStr(static_cast<Protocol>(0)), "unknown");
}

TEST(ProtocolTests, ProtocolHeaderValidatesKnownProtocols) {
    const ProtocolHeader rspHeader{
        Protocol::rsp, ProtocolHeader::magicResponse, {}
    };
    const ProtocolHeader jfpHeader{
        Protocol::jfp, ProtocolHeader::magicRequest, {}
    };
    const ProtocolHeader invalidHeader{
        static_cast<Protocol>(0), ProtocolHeader::magicRequest, {}
    };

    EXPECT_TRUE(rspHeader.protocolValid());
    EXPECT_TRUE(jfpHeader.protocolValid());
    EXPECT_FALSE(invalidHeader.protocolValid());

    const ProtocolHeader invalidAgain{
        static_cast<Protocol>(0), ProtocolHeader::magicRequest, {}
    };
    EXPECT_FALSE(invalidAgain.protocolValid());
}
