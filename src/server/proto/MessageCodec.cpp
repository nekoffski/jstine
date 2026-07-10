#include "MessageCodec.hh"

#include "core/Profiler.hh"
#include "impl/JFP.hh"

namespace jstine {

MessageCodec::MessageCodec(Protocol protocol) : m_protocol(protocol) {
    if (protocol == Protocol::rsp) {
        log::panic("RSP protocol is not yet implemented");
    } else if (protocol == Protocol::jfp) {
        m_decoder = std::make_unique<JFPRequestDecoder>();
        m_encoder = std::make_unique<JFPResponseEncoder>();
    }
}

void MessageCodec::feed(std::span<const Byte> bytes) {
    JSTINE_PROFILE_FUNCTION();
    m_decoder->feed(bytes);
}

Result<Request> MessageCodec::decode() {
    JSTINE_PROFILE_FUNCTION();
    return m_decoder->decode();
}

Result<u64> MessageCodec::encode(
    const Response& response, std::span<Byte> bytes
) {
    JSTINE_PROFILE_FUNCTION();
    return m_encoder->encode(response, bytes);
}

Protocol MessageCodec::protocol() const { return m_protocol; }

}  // namespace jstine
