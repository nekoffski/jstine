#pragma once

#include "ExpirationRegistry.hh"
#include "Key.hh"
#include "Keyspace.hh"
#include "Value.hh"
#include "core/Concepts.hh"
#include "core/Config.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "mem/Mallocator.hh"

namespace jstine {

class Database : public NonCopyable, public NonMovable {
   public:
    explicit Database(
        const Config& config, Keyspace& keyspace,
        ExpirationRegistry& expirationRegistry
    );

    bool exists(std::span<const Byte> keyBytes) const;
    void remove(std::span<const Byte> keyBytes);
    Opt<Error> set(
        std::span<const Byte> keyBytes, std::span<const Byte> valueBytes
    );
    Result<std::span<const Byte>> get(std::span<const Byte> keyBytes) const;

   private:
    const Config& m_config;
    Keyspace& m_keyspace;
    ExpirationRegistry& m_expirationRegistry;
    Mallocator m_mallocator;
};

}  // namespace jstine
