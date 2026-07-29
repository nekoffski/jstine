#pragma once

#include <array>

#include "Asio.hh"
#include "AsioMessageHandler.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "proto/Message.hh"
#include "proto/MessageCodec.hh"
#include "proto/Protocol.hh"
#include "storage/api/StorageCommandQueue.hh"

namespace jstine {

class AsioSession : public NonCopyable, public NonMovable {
   public:
    using Buffer = std::array<Byte, 1024>;

    explicit AsioSession(
        asio::ip::tcp::socket socket, StorageCommandQueue& commandQueue
    );

    asio::awaitable<void> start();

   private:
    void logError(const std::string& op, const Error& error);
    void logOp(const std::string& op, u64 bytes);

    asio::awaitable<Result<u64>> read();
    asio::awaitable<Result<u64>> write(u64 bytesToWrite);

    asio::awaitable<Result<Request>> readRequest(MessageCodec& codec);
    asio::awaitable<Opt<Error>> writeResponse(
        const Response& response, MessageCodec& codec
    );

    asio::awaitable<Result<Protocol>> establishProtocol();

    asio::ip::tcp::socket m_socket;
    AsioMessageHandler m_messageHandler;
    Str m_ident;
    Buffer m_buffer;
};

}  // namespace jstine