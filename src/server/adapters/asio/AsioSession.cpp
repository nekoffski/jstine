#include "AsioSession.hh"

#include "core/Log.hh"
#include "core/Scope.hh"
#include "core/String.hh"
#include "proto/MessageCodec.hh"

namespace jstine {

namespace {

std::string extractIdent(asio::ip::tcp::socket& socket) {
    return fmt::format(
        "{}:{}", socket.remote_endpoint().address().to_string(),
        socket.remote_endpoint().port()
    );
}

Error convertError(const std::error_code& ec) {
    if (ec == asio::error::eof) {
        return Error{ErrorCode::eof, "Socket closed by peer"};
    }

    return Error{ErrorCode::connectionFailure, ec.message()};
}

}  // namespace

AsioSession::AsioSession(asio::ip::tcp::socket socket)
    : m_socket(std::move(socket)), m_ident(extractIdent(m_socket)) {}

asio::awaitable<void> AsioSession::start() {
    log::info("{} - session started", m_ident);
    ON_SCOPE_EXIT { log::info("{} - session ended", m_ident); };

    auto protocol = co_await establishProtocol();

    if (not protocol) {
        log::error(
            "{} - failed to establish protocol: {}", m_ident,
            protocol.error().message()
        );
        co_return;
    }

    MessageCodec codec{protocol.value()};

    auto& encoder = codec.encoder();
    auto& decoder = codec.decoder();

    for (;;) {
        auto request = co_await readRequest(decoder);

        if (not request) {
            logError("recv", request.error());
            co_return;
        }

        // mock for now!
        // auto response = Response{ResponseKind::ok};

        // if (auto r = co_await writeResponse(response, encoder); not r) {
        //     logError("send", r.error());
        //     co_return;
        // }
    }
}
asio::awaitable<Result<Request>> AsioSession::readRequest(
    RequestDecoder& decoder
) {
    auto bytes = co_await read();

    if (not bytes) {
        co_return Error::unexpected(bytes.error());
    }

    co_return Request{RequestKind::ping, RequestBody{}};
}

asio::awaitable<Result<Response>> AsioSession::writeResponse(
    const Response& response, ResponseEncoder& encoder
) {}

asio::awaitable<Result<Protocol>> AsioSession::establishProtocol() {
    ProtocolHeader handshake;

    log::debug("{} - establishing protocol", m_ident);

    auto [ec, bytes] = co_await asio::async_read(
        m_socket, asio::buffer(&handshake, sizeof(ProtocolHeader)),
        asio::as_tuple(asio::use_awaitable)
    );

    log::debug(
        "{} - read {}/{} bytes for handshake", m_ident, bytes,
        sizeof(ProtocolHeader)
    );

    if (ec) {
        co_return Error::unexpected(convertError(ec));
    }

    if (handshake.magic != ProtocolHeader::magicRequest) {
        co_return Error::unexpected(
            ErrorCode::handshakeFailure,
            "Invalid handshake magic: expected 0x{:08x}, got 0x{:08x}",
            ProtocolHeader::magicRequest, handshake.magic
        );
    }

    if (not handshake.protocolValid()) {
        co_return Error::unexpected(
            ErrorCode::handshakeFailure, "Invalid handshake protocol: {}",
            protocolToStr(handshake.protocol)
        );
    }

    handshake.magic = ProtocolHeader::magicResponse;

    log::debug(
        "{} - handshake valid, sending response for protocol {}", m_ident,
        protocolToStr(handshake.protocol)
    );

    std::tie(ec, bytes) = co_await asio::async_write(
        m_socket, asio::buffer(&handshake, sizeof(ProtocolHeader)),
        asio::as_tuple(asio::use_awaitable)
    );

    if (ec) {
        co_return Error::unexpected(convertError(ec));
    }

    co_return handshake.protocol;
}

asio::awaitable<Result<u64>> AsioSession::read() {
    auto [ec, bytesRead] = co_await m_socket.async_read_some(
        asio::buffer(m_buffer), asio::as_tuple(asio::use_awaitable)
    );

    if (ec) {
        co_return Error::unexpected(convertError(ec));
    }

    logOp("recv", bytesRead);
    co_return bytesRead;
}

asio::awaitable<Result<u64>> AsioSession::write(u64 bytesToWrite) {
    auto [ec, bytesWritten] = co_await asio::async_write(
        m_socket, asio::buffer(m_buffer.data(), bytesToWrite),
        asio::as_tuple(asio::use_awaitable)
    );

    if (ec) {
        co_return Error::unexpected(convertError(ec));
    }

    logOp("send", bytesWritten);
    co_return bytesWritten;
}

void AsioSession::logError(const std::string& op, const Error& error) {
    if (error.code() == ErrorCode::eof) {
        log::info("{} ({}) connection closed by peer", m_ident, op);
    } else {
        log::error("{} ({}) error on socket: {}", m_ident, op, error.message());
    }
}

void jstine::AsioSession::logOp(const std::string& op, u64 bytes) {
    if (log::level() >= log::Level::debug) [[unlikely]] {
        log::debug(
            "{} - {} ({}) - '{}'", m_ident, op, bytes,
            hexDump({m_buffer.data(), bytes})
        );
    }
}

}  // namespace jstine
