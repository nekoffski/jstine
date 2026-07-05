#include "MessageCodec.hh"

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

Protocol MessageCodec::protocol() const { return m_protocol; }

RequestDecoder& MessageCodec::decoder() { return *m_decoder; }

ResponseEncoder& MessageCodec::encoder() { return *m_encoder; }

}  // namespace jstine
