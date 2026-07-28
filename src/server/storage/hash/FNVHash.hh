#pragma once

#include "core/Core.hh"
#include "storage/kv/Key.hh"

namespace jstine {

struct FNVKeyHashFunctor {
    u64 operator()(const Key& v) const noexcept;
};

}  // namespace jstine