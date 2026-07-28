#pragma once

#include "core/Concepts.hh"
#include "core/Core.hh"

namespace jstine {

class Key {
   public:
    explicit Key(Bytes&& bytes);
    explicit Key(const Bytes& bytes);
    explicit Key(std::span<const Byte> bytes);

    const Bytes& bytes() const;

    bool operator==(const Key& other) const;

   private:
    Bytes m_bytes;
};

}  // namespace jstine
