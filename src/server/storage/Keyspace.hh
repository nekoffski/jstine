#pragma once

#include "Key.hh"
#include "Value.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class Keyspace : public NonCopyable, public NonMovable {
   public:
    virtual ~Keyspace() = default;

    virtual bool exists(const Key& key) const = 0;
    virtual void remove(const Key& key) = 0;
    virtual Opt<Error> set(const Key& key, const Value& value) = 0;
    virtual Result<Value> get(const Key& key) const = 0;
};

}  // namespace jstine
