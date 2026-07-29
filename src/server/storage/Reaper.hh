#pragma once

#include <condition_variable>
#include <mutex>

#include "ExpirationRegistry.hh"
#include "api/StorageProxy.hh"
#include "core/Config.hh"
#include "keyspace/Keyspace.hh"
#include "runtime/Thread.hh"

namespace jstine {

class Reaper : public Thread {
   public:
    explicit Reaper(
        const Config& config, Keyspace& keyspace,
        const ExpirationRegistry& expirationRegistry, StorageProxy& storageProxy
    );

    void cancel() override;

   private:
    void run() override;
    void reap();

    std::atomic_bool m_running{true};
    const Config& m_config;
    Keyspace& m_keyspace;
    const ExpirationRegistry& m_expirationRegistry;
    StorageProxy& m_storageProxy;

    std::mutex m_mutex;
    std::condition_variable m_cv;
};

}  // namespace jstine
