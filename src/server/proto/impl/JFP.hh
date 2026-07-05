#pragma once

#include "core/Core.hh"
#include "proto/RequestDecoder.hh"
#include "proto/ResponseEncoder.hh"

namespace jstine {

// JFP wire format:
//   Frame = [u32 payload_size][u32 kind][Field...]
//   Field = [u8 type][u32 size][data...]
//   payload_size = 4(kind) + sum(5 + field.size)
//
// Field types:
//   1 = payload       PingRequest payload / OkResponse payload
//   2 = key           Get/Set/Del/Exists key
//   3 = value         Set value
//   4 = error_code    ErrorResponse code (u32 LE)
//   5 = error_message ErrorResponse message (utf-8 bytes)

enum class JFPFieldType : u8 {
    payload = 1,
    key = 2,
    value = 3,
    errorCode = 4,
    errorMessage = 5,
};

class JFPRequestDecoder : public RequestDecoder {
   public:
    void feed(std::span<const Byte> bytes) override;
    Result<Request> decode() override;

   private:
    Bytes m_buf;
};

class JFPResponseEncoder : public ResponseEncoder {
   public:
    Result<u64> encode(const Response& response, std::span<Byte> out) override;
};

}  // namespace jstine
