#include "AsioServer.hh"

#include "AsioSession.hh"
#include "runtime/Thread.hh"

namespace jstine {

AsioServer::AsioServer(const Config& cfg, MessageHandler& handler)
    : Thread("AsioMain"),
      m_io(cfg.api().concurrency),
      m_cfg(cfg),
      m_messageHandler(handler),
      m_acceptor(m_io, {asio::ip::tcp::v4(), cfg.api().port}) {}

void AsioServer::run() {
    asio::co_spawn(
        m_io,
        [&]() -> asio::awaitable<void> {
            for (;;) {
                co_await acceptConnection();
            }
        },
        asio::detached
    );

    if (const auto threads = m_cfg.api().concurrency; threads > 1) {
        log::info("ASIO server starting with {} threads", threads);

        ThreadGroup tg;
        for (int i = 0; i < threads; ++i) {
            tg.add(fmt::format("AsioWorker{}", i + 1), [&] { m_io.run(); });
        }

        if (const auto err = tg.start(); err) {
            log::panic("failed to start thread group: {}", err->message());
            return;
        }

        tg.join();
    } else {
        log::info("ASIO server starting single-threaded");
        m_io.run();
    }
}

void AsioServer::cancel() { m_io.stop(); }

asio::awaitable<void> AsioServer::acceptConnection() {
    auto [ec, socket] =
        co_await m_acceptor.async_accept(asio::as_tuple(asio::use_awaitable));

    if (ec) {
        log::error("failed to accept connection: {}", ec.message());
        co_return;
    }

    auto ex = co_await asio::this_coro::executor;

    asio::co_spawn(
        ex,
        [&, s = std::move(socket)]() mutable -> asio::awaitable<void> {
            AsioSession session{std::move(s), m_messageHandler};
            co_return co_await session.start();
        },
        asio::detached
    );
}

}  // namespace jstine
