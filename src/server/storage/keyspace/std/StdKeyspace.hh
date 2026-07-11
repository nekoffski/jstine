#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "storage/Keyspace.hh"
#include "storage/hash/FNV.hh"

namespace jstine {

class StdKeyspace : public Keyspace {
   public:
    bool exists(const Key& key) const override;
    void remove(const Key& key) override;
    Opt<Error> set(const Key& key, const Value& value) override;
    Result<Value> get(const Key& key) const override;

   private:
    std::unordered_map<Key, Value, FNVKeyHashFunctor> m_storage;
    mutable std::shared_mutex m_storageMutex;
};

}  // namespace jstine
