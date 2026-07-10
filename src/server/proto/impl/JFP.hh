#pragma once

#include "core/Core.hh"
#include "proto/RequestDecoder.hh"
#include "proto/ResponseEncoder.hh"

namespace jstine {

// JFP is a length-prefixed binary protocol.
//
// Request/response frames share the same outer shape:
//   frame = [u32 payload_size][u32 kind][field...]
//
// `payload_size` counts bytes after the size prefix itself. In other words:
//   payload_size = 4(kind) + sum(field_header + field_data)
//
// Each field is encoded as:
//   field = [u8 type][u32 size][size bytes of payload]
//
// The field type determines how the payload is interpreted:
//   1 = payload       Ping request payload / Ok response payload
//   2 = key           Key used by get/set/delete/exists
//   3 = value         Value used by set
//   4 = error_code    Error response code, little-endian u32
//   5 = error_message Error response message, utf-8 bytes
//
// The decoder accepts incremental feeds:
// - `feed()` appends raw bytes to an internal buffer
// - `decode()` returns `requestNotReady` until one full frame is available
// - once a full frame exists, it consumes exactly one request from the buffer
//   and leaves any trailing bytes for the next `decode()` call

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
