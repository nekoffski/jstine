#include "Server.hh"

#include "adapters/asio/AsioServer.hh"
#include "core/Scope.hh"

namespace jstine {

Server::Server(const ServerContext& context)
    : m_context(context),
      m_storageEngine(context.config),
      m_messageHandler(m_storageEngine.database()) {}

Opt<Error> Server::run() {
    if (auto err = initSignals(); err) {
        return err;
    }
    ON_SCOPE_EXIT { deinitSignals(); };

    buildServices();

    if (auto err = m_threadGroup.start(); err) {
        return err;
    }

    m_threadGroup.join();
    return Error::empty();
}

void Server::buildServices() {
    m_threadGroup.add<AsioServer>(m_context.config, m_messageHandler);
}

Opt<Error> Server::initSignals() {
    return m_context.signals.registerHandler(Signal::interrupt, []() {
        log::info("SIGINT received, stopping server..");
        Server::instance().stop();
    });
}

void Server::deinitSignals() {
    m_context.signals.removeHandler(Signal::interrupt);
}

void Server::stop() {
    log::info("stopping server");
    m_threadGroup.cancel();
}

}  // namespace jstine
