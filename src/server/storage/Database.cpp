#include "Database.hh"

namespace jstine {

Database::Database(Keyspace& keyspace) : m_keyspace(keyspace) {}

bool Database::exists(const Key& key) const { return m_keyspace.exists(key); }

void Database::remove(const Key& key) { m_keyspace.remove(key); }

Opt<Error> Database::set(const Key& key, const Value& value) {
    return m_keyspace.set(key, value);
}

Result<Value> Database::get(const Key& key) const {
    return m_keyspace.get(key);
}

}  // namespace jstine
