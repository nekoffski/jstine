#include "StorageManager.hh"

namespace jstine {

bool StorageManager::exists(const Key& key) const {
    return m_storage.contains(key);
}

void StorageManager::remove(const Key& key) { m_storage.erase(key); }

Opt<Error> StorageManager::set(const Key& key, const Value& value) {
    m_storage[key] = value;
    return {};
}

Result<Value> StorageManager::get(const Key& key) const {
    if (not exists(key)) {
        return Error::unexpected(ErrorCode::notFound, "Key does not exist");
    }
    return m_storage.at(key);
}

}  // namespace jstine
