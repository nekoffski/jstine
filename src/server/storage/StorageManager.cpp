#include "StorageManager.hh"

#include "std/StdStorageManager.hh"

namespace jstine {

std::unique_ptr<StorageManager> StorageManager::create(const Config&) {
    return std::make_unique<StdStorageManager>();
}

}  // namespace jstine
