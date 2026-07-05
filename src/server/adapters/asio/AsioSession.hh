#pragma once

#include <array>

#include "Asio.hh"
#include "api/Message.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "proto/Protocol.hh"
#include "proto/RequestDecoder.hh"
#include "proto/ResponseEncoder.hh"

namespace jstine {

class AsioSession : public NonCopyable, public NonMovable {
   public:
    using Buffer = std::array<Byte, 1024>;

    explicit AsioSession(asio::ip::tcp::socket socket);

    asio::awaitable<void> start();

   private:
    void logError(const std::string& op, const Error& ec);
    void logOp(const std::string& op, u64 bytes);

    asio::awaitable<Result<u64>> read();
    asio::awaitable<Result<u64>> write(u64 bytesToWrite);

    asio::awaitable<Result<Request>> readRequest(RequestDecoder& decoder);
    asio::awaitable<Result<Response>> writeResponse(
        const Response& response, ResponseEncoder& encoder
    );

    asio::awaitable<Result<Protocol>> establishProtocol();

    asio::ip::tcp::socket m_socket;
    Str m_ident;
    Buffer m_buffer;
};

}  // namespace jstine