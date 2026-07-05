#pragma once

#include "proto/RequestDecoder.hh"
#include "proto/ResponseEncoder.hh"

namespace jstine {

class JFPRequestDecoder : public RequestDecoder {
   public:
    Result<Request> decode() override;
};

class JFPResponseEncoder : public ResponseEncoder {
   public:
    Result<u64> encode(const Response& response, std::span<Byte> bytes)
        override;
};

}  // namespace jstine
