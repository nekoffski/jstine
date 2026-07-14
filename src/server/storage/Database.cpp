#include "Database.hh"

namespace jstine {

Database::Database(
    const Config& config, Keyspace& keyspace,
    ExpirationRegistry& expirationRegistry
)
    : m_config(config),
      m_keyspace(keyspace),
      m_expirationRegistry(expirationRegistry) {}

bool Database::exists(const Bytes& keyBytes) const {
    Key key{keyBytes};

    if (m_expirationRegistry.expired(key)) {
        m_keyspace.remove(key);
        return false;
    }
    return m_keyspace.exists(key);
}

void Database::remove(const Bytes& keyBytes) {
    m_keyspace.remove(Key{keyBytes});
}

Opt<Error> Database::set(const Bytes& keyBytes, const Bytes& valueBytes) {
    Key key{keyBytes};

    auto value = Value::fromBytes(valueBytes);

    if (not value) {
        return value.error();
    }

    if (auto err = m_keyspace.set(key, *value); err) {
        return err;
    }

    m_expirationRegistry.expiresAfter(
        key, m_config.storage().defaultExpiration
    );
    return Error::empty();
}

Result<Bytes> Database::get(const Bytes& keyBytes) const {
    Key key{keyBytes};
    if (m_expirationRegistry.expired(key)) {
        m_keyspace.remove(key);
        return Error::unexpected(ErrorCode::notFound, "Key has expired");
    }
    if (auto value = m_keyspace.get(key); value) {
        return value->bytes();
    } else {
        return Error::unexpected(value.error());
    }
}

}  // namespace jstine
