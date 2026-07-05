#pragma once

#include <memory>
#include <span>

#include "Protocol.hh"
#include "RequestDecoder.hh"
#include "ResponseEncoder.hh"
#include "api/Message.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class MessageCodec : public NonCopyable {
   public:
    explicit MessageCodec(Protocol protocol);

    Protocol protocol() const;

    RequestDecoder& decoder();
    ResponseEncoder& encoder();

   private:
    std::unique_ptr<RequestDecoder> m_decoder;
    std::unique_ptr<ResponseEncoder> m_encoder;

    Protocol m_protocol;
};

}  // namespace jstine
