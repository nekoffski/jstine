#pragma once

#include <memory>

#include "Database.hh"
#include "ExpirationRegistry.hh"
#include "Reaper.hh"
#include "core/Concepts.hh"
#include "core/Config.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "keyspace/Keyspace.hh"
#include "kv/Key.hh"
#include "kv/Value.hh"

namespace jstine {

class StorageEngine : public NonCopyable, public NonMovable {
   public:
    explicit StorageEngine(const Config& config);

    Storage& storage();

    void start();
    void stop();

   private:
    const Config& m_config;
    std::unique_ptr<Keyspace> m_keyspace;
    ExpirationRegistry m_expirationRegistry;
    Database m_database;
    Reaper m_reaper;
};

}  // namespace jstine
