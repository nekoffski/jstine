#include "Database.hh"

namespace jstine {

Database::Database(
    const Config& config, Keyspace& keyspace,
    ExpirationRegistry& expirationRegistry
)
    : m_config(config),
      m_keyspace(keyspace),
      m_expirationRegistry(expirationRegistry) {}

bool Database::exists(const Key& key) const {
    if (m_expirationRegistry.expired(key)) {
        m_keyspace.remove(key);
        return false;
    }
    return m_keyspace.exists(key);
}

void Database::remove(const Key& key) { m_keyspace.remove(key); }

Opt<Error> Database::set(const Key& key, const Value& value) {
    if (auto err = m_keyspace.set(key, value); err) {
        return err;
    }
    m_expirationRegistry.expiresAfter(
        key, m_config.storage().defaultExpiration
    );
    return Error::empty();
}

Result<Value> Database::get(const Key& key) const {
    if (m_expirationRegistry.expired(key)) {
        m_keyspace.remove(key);
        return Error::unexpected(ErrorCode::notFound, "Key has expired");
    }
    return m_keyspace.get(key);
}

}  // namespace jstine
