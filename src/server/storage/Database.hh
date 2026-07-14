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

    bool exists(const Bytes& keyBytes) const;
    void remove(const Bytes& keyBytes);
    Opt<Error> set(const Bytes& keyBytes, const Bytes& valueBytes);
    Result<Bytes> get(const Bytes& keyBytes) const;

   private:
    const Config& m_config;
    Keyspace& m_keyspace;
    ExpirationRegistry& m_expirationRegistry;
};

}  // namespace jstine
