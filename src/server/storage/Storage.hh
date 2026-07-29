#pragma once

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class Storage : public NonCopyable, public NonMovable {
   public:
    virtual ~Storage() = default;

    virtual bool exists(std::span<const Byte> keyBytes) const = 0;
    virtual void remove(std::span<const Byte> keyBytes) = 0;
    virtual Opt<Error> set(
        std::span<const Byte> keyBytes, std::span<const Byte> valueBytes
    ) = 0;
    virtual Result<std::span<const Byte>> get(
        std::span<const Byte> keyBytes
    ) const = 0;
};

}  // namespace jstine
