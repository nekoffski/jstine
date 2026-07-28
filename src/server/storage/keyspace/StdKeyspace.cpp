#include "StdKeyspace.hh"

namespace jstine {

bool StdKeyspace::exists(const Key& key) const {
    std::shared_lock lk{m_storageMutex};
    return m_storage.contains(key);
}

void StdKeyspace::remove(const Key& key) {
    std::unique_lock lk{m_storageMutex};
    m_storage.erase(key);
}

Result<std::reference_wrapper<ValueWrapper>> StdKeyspace::reserve(
    const Key& key
) {
    std::unique_lock lk{m_storageMutex};

    if (auto it = m_storage.find(key); it != m_storage.end()) {
        return it->second;
    } else {
        m_storage.emplace(key, ValueWrapper{});
    }
    return m_storage[key];
}

ValueWrapper* StdKeyspace::get(const Key& key) {
    std::shared_lock lk{m_storageMutex};
    if (auto it = m_storage.find(key); it != m_storage.end()) {
        return &it->second;
    }
    return nullptr;
}

}  // namespace jstine
