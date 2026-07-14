#include "StorageEngine.hh"

#include "keyspace/StdKeyspace.hh"

namespace jstine {

StorageEngine::StorageEngine(const Config& config)
    : m_config(config),
      m_keyspace(std::make_unique<StdKeyspace>()),
      m_database(config, *m_keyspace, m_expirationRegistry),
      m_reaper(config, *m_keyspace, m_expirationRegistry) {}

Database& StorageEngine::database() { return m_database; }

void StorageEngine::start() { m_reaper.start(); }

void StorageEngine::stop() {
    m_reaper.cancel();
    m_reaper.join();
}

}  // namespace jstine
