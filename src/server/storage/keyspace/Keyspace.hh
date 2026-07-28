#pragma once

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "storage/kv/Key.hh"
#include "storage/kv/ValueWrapper.hh"

namespace jstine {

class Keyspace : public NonCopyable, public NonMovable {
   public:
    virtual ~Keyspace() = default;

    virtual bool exists(const Key& key) const = 0;
    virtual void remove(const Key& key) = 0;
    virtual Result<std::reference_wrapper<ValueWrapper>> reserve(
        const Key& key
    ) = 0;
    virtual ValueWrapper* get(const Key& key) = 0;
};

}  // namespace jstine
