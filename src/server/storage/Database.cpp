#include "Database.hh"

namespace jstine {

Database::Database(
    const Config& config, Keyspace& keyspace,
    ExpirationRegistry& expirationRegistry
)
    : m_config(config),
      m_keyspace(keyspace),
      m_expirationRegistry(expirationRegistry) {}

bool Database::exists(std::span<const Byte> keyBytes) const {
    Key key{keyBytes};

    if (m_expirationRegistry.expired(key)) {
        m_keyspace.remove(key);
        return false;
    }
    return m_keyspace.exists(key);
}

void Database::remove(std::span<const Byte> keyBytes) {
    m_keyspace.remove(Key{keyBytes});
}

Opt<Error> Database::set(
    std::span<const Byte> keyBytes, std::span<const Byte> valueBytes
) {
    Key key{keyBytes};

    auto value = Value::fromBytes(valueBytes, m_mallocator);

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

Result<std::span<const Byte>> Database::get(
    std::span<const Byte> keyBytes
) const {
    Key key{keyBytes};
    if (m_expirationRegistry.expired(key)) {
        m_keyspace.remove(key);
        return Error::unexpected(ErrorCode::notFound, "Key has expired");
    }
    if (auto value = m_keyspace.get(key); value) {
        return value->bytes();
    } else {
        return Error::unexpected(ErrorCode::notFound, "Key does not exist");
    }
}

}  // namespace jstine
