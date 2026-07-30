#pragma once

#include "ExpirationRegistry.hh"
#include "core/Concepts.hh"
#include "core/Config.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "keyspace/Keyspace.hh"
#include "kv/Key.hh"
#include "kv/Value.hh"
#include "mem/Mallocator.hh"

namespace jstine {

class Database : public NonCopyable, public NonMovable {
   public:
    explicit Database(
        const Config& config, Keyspace& keyspace,
        ExpirationRegistry& expirationRegistry
    );

    bool exists(CBytesView keyBytes) const;
    bool remove(const std::vector<CBytesView>& keyBytesList);
    Opt<Error> set(CBytesView keyBytes, CBytesView valueBytes);
    Result<CBytesView> get(CBytesView keyBytes) const;

   private:
    const Config& m_config;
    Keyspace& m_keyspace;
    ExpirationRegistry& m_expirationRegistry;
    Mallocator m_mallocator;
};

}  // namespace jstine
