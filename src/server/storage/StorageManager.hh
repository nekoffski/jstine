#pragma once

#include <unordered_map>

#include "Storage.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

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

class StorageManager : public NonCopyable, public NonMovable {
   public:
    bool exists(const Key& key) const;
    void remove(const Key& key);
    Opt<Error> set(const Key& key, const Value& value);
    Result<Value> get(const Key& key) const;

   private:
    std::unordered_map<Key, Value, VecU8Hash> m_storage;
};

}  // namespace jstine
