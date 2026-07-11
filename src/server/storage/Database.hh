#pragma once

#include "ExpirationRegistry.hh"
#include "Key.hh"
#include "Keyspace.hh"
#include "Value.hh"
#include "core/Concepts.hh"
#include "core/Config.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class Database : public NonCopyable, public NonMovable {
   public:
    explicit Database(
        const Config& config, Keyspace& keyspace,
        ExpirationRegistry& expirationRegistry
    );

    bool exists(const Key& key) const;
    void remove(const Key& key);
    Opt<Error> set(const Key& key, const Value& value);
    Result<Value> get(const Key& key) const;

   private:
    const Config& m_config;
    Keyspace& m_keyspace;
    ExpirationRegistry& m_expirationRegistry;
};

}  // namespace jstine
