#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "storage/Storage.hh"
#include "storage/StorageManager.hh"

namespace jstine {

struct VecU8Hash {
    size_t operator()(const std::vector<uint8_t>& v) const noexcept {
        size_t hash = 1469598103934665603ull;  // FNV offset basis

        for (uint8_t b : v) {
            hash ^= static_cast<size_t>(b);
            hash *= 1099511628211ull;  // FNV prime
        }

        return hash;
    }
};

class StdStorageManager : public StorageManager {
   public:
    bool exists(const Key& key) const override;
    void remove(const Key& key) override;
    Opt<Error> set(const Key& key, const Value& value) override;
    Result<Value> get(const Key& key) const override;

   private:
    std::unordered_map<Key, Value, VecU8Hash> m_storage;
    mutable std::shared_mutex m_storageMutex;
};

}  // namespace jstine
