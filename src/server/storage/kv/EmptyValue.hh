#pragma once

#include "core/Core.hh"

namespace jstine {

class EmptyValue {
   public:
    std::span<const Byte> bytes() const;
};

}  // namespace jstine
