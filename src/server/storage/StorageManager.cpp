#include "StorageManager.hh"

namespace jstine {

bool StorageManager::exists(const Key& key) const { return false; }

void StorageManager::remove(const Key& key) {}

Opt<Error> StorageManager::set(const Key& key, const Value& value) {
    return Error{
        ErrorCode::notImplementedYet, "StorageManager::set not implemented yet"
    };
}

}  // namespace jstine
