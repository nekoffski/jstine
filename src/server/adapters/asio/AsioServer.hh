#pragma once

#include "Asio.hh"
#include "core/Config.hh"
#include "runtime/Thread.hh"

namespace jstine {

class AsioServer : public Thread {
   public:
    explicit AsioServer(const Config& cfg);

   private:
    void run() override;
    void cancel() override;

    asio::awaitable<void> acceptConnection();

    const Config& m_cfg;

    asio::io_context m_io;
    asio::ip::tcp::acceptor m_acceptor;
};

}  // namespace jstine
