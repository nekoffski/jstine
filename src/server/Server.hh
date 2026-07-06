#pragma once

#include "api/MessageHandler.hh"
#include "core/Concepts.hh"
#include "core/Config.hh"
#include "core/Core.hh"
#include "core/Singleton.hh"
#include "runtime/Signal.hh"
#include "runtime/Thread.hh"
#include "storage/StorageManager.hh"

namespace jstine {

struct ServerContext {
    SignalManager& signals;
    const Config& config;
};

class Server : public UniqueInstance<Server> {
   public:
    explicit Server(const ServerContext& context);

    Opt<Error> run();

   private:
    void stop();

    void buildServices();

    [[nodiscard]] Opt<Error> initSignals();
    void deinitSignals();

    ServerContext m_context;
    ThreadGroup m_threadGroup;
    std::unique_ptr<StorageManager> m_storageManager;
    MessageHandler m_messageHandler;
};

}  // namespace jstine
