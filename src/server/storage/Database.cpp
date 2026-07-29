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

bool Database::remove(std::span<const Byte> keyBytes) {
    m_keyspace.remove(Key{keyBytes});
    return true;
}

Opt<Error> Database::set(
    std::span<const Byte> keyBytes, std::span<const Byte> valueBytes
) {
    Key key{keyBytes};

    auto maybeWrapper = m_keyspace.reserve(key);

    if (not maybeWrapper) {
        return maybeWrapper.error();
    }

    auto& wrapper = maybeWrapper->get();

    if (auto err = wrapper.set(valueBytes, m_mallocator); err) {
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
    if (auto wrapper = m_keyspace.get(key); wrapper) {
        return wrapper->bytes();
    } else {
        return Error::unexpected(ErrorCode::notFound, "Key does not exist");
    }
}

}  // namespace jstine
