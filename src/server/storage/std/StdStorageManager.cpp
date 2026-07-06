#include "StdStorageManager.hh"

namespace jstine {

bool StdStorageManager::exists(const Key& key) const {
    return m_storage.contains(key);
}

void StdStorageManager::remove(const Key& key) { m_storage.erase(key); }

Opt<Error> StdStorageManager::set(const Key& key, const Value& value) {
    m_storage[key] = value;
    return {};
}

Result<Value> StdStorageManager::get(const Key& key) const {
    if (not exists(key)) {
        return Error::unexpected(ErrorCode::notFound, "Key does not exist");
    }
    return m_storage.at(key);
}

}  // namespace jstine
