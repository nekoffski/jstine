#include "StdStorageManager.hh"

namespace jstine {

bool StdStorageManager::exists(const Key& key) const {
    std::shared_lock lk{m_storageMutex};
    return m_storage.contains(key);
}

void StdStorageManager::remove(const Key& key) {
    std::unique_lock lk{m_storageMutex};
    m_storage.erase(key);
}

Opt<Error> StdStorageManager::set(const Key& key, const Value& value) {
    std::unique_lock lk{m_storageMutex};
    m_storage[key] = value;
    return {};
}

Result<Value> StdStorageManager::get(const Key& key) const {
    std::shared_lock lk{m_storageMutex};
    if (auto it = m_storage.find(key); it != m_storage.end()) {
        return it->second;
    }
    return Error::unexpected(ErrorCode::notFound, "Key does not exist");
}

}  // namespace jstine
