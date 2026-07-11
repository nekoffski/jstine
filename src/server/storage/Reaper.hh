#pragma once

#include <condition_variable>
#include <mutex>

#include "ExpirationRegistry.hh"
#include "Keyspace.hh"
#include "core/Config.hh"
#include "runtime/Thread.hh"

namespace jstine {

class Reaper : public Thread {
   public:
    explicit Reaper(
        const Config& config, Keyspace& keyspace,
        const ExpirationRegistry& expirationRegistry
    );

    void cancel() override;

   private:
    void run() override;
    void reap();

    std::atomic_bool m_running{true};
    const Config& m_config;
    Keyspace& m_keyspace;
    const ExpirationRegistry& m_expirationRegistry;

    std::mutex m_mutex;
    std::condition_variable m_cv;
};

}  // namespace jstine
