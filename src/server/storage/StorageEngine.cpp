#include "StorageEngine.hh"

#include "keyspace/StdKeyspace.hh"

namespace jstine {

StorageEngine::StorageEngine(const Config& config)
    : m_config(config),
      m_commandQueue(m_commandQueueInternal),
      m_keyspace(std::make_unique<StdKeyspace>()),
      m_database(config, *m_keyspace, m_expirationRegistry),
      m_executor(m_database, m_commandQueueInternal),
      m_reaper(config, *m_keyspace, m_expirationRegistry) {}

StorageCommandQueue& StorageEngine::commandQueue() { return m_commandQueue; }

void StorageEngine::start() {
    m_executor.start();
    m_reaper.start();
}

void StorageEngine::stop() {
    m_reaper.cancel();
    m_reaper.join();
    m_executor.cancel();
    m_executor.join();
}

}  // namespace jstine
