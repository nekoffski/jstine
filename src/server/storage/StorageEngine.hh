#pragma once

#include <memory>

#include "Database.hh"
#include "ExpirationRegistry.hh"
#include "Reaper.hh"
#include "api/Commands.hh"
#include "api/StorageCommandQueue.hh"
#include "api/StorageExecutor.hh"
#include "api/StorageProxy.hh"
#include "core/Concepts.hh"
#include "core/Config.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "keyspace/Keyspace.hh"
#include "kv/Key.hh"
#include "kv/Value.hh"
#include "runtime/ThreadSafeQueue.hh"

namespace jstine {

class StorageEngine : public NonCopyable, public NonMovable {
   public:
    explicit StorageEngine(const Config& config);

    StorageCommandQueue& commandQueue();

    void start();
    void stop();

   private:
    const Config& m_config;

    ThreadSafeQueue<Command> m_commandQueueInternal;
    StorageCommandQueue m_commandQueue;
    StorageProxy m_storageProxy;
    std::unique_ptr<Keyspace> m_keyspace;
    ExpirationRegistry m_expirationRegistry;
    Database m_database;
    StorageExecutor m_executor;
    Reaper m_reaper;
};

}  // namespace jstine
