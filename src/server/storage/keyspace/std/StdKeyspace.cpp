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

Opt<Error> StdKeyspace::set(const Key& key, const Value& value) {
    std::unique_lock lk{m_storageMutex};
    m_storage[key] = value;
    return {};
}

Result<Value> StdKeyspace::get(const Key& key) const {
    std::shared_lock lk{m_storageMutex};
    if (auto it = m_storage.find(key); it != m_storage.end()) {
        return it->second;
    }
    return Error::unexpected(ErrorCode::notFound, "Key does not exist");
}

}  // namespace jstine
