#pragma once

#include "Key.hh"
#include "Keyspace.hh"
#include "Value.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class Database : public NonCopyable, public NonMovable {
   public:
    explicit Database(Keyspace& keyspace);

    bool exists(const Key& key) const;
    void remove(const Key& key);
    Opt<Error> set(const Key& key, const Value& value);
    Result<Value> get(const Key& key) const;

   private:
    Keyspace& m_keyspace;
};

}  // namespace jstine
