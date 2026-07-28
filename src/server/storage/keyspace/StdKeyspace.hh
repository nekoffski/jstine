#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "Keyspace.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "storage/hash/FNVHash.hh"

namespace jstine {

class StdKeyspace : public Keyspace {
   public:
    bool exists(const Key& key) const override;
    void remove(const Key& key) override;
    Result<std::reference_wrapper<ValueWrapper>> reserve(
        const Key& key
    ) override;
    ValueWrapper* get(const Key& key) override;

   private:
    std::unordered_map<Key, ValueWrapper, FNVKeyHashFunctor> m_storage;
    mutable std::shared_mutex m_storageMutex;
};

}  // namespace jstine
