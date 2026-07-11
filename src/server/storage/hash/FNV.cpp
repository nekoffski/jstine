#include "FNV.hh"

namespace jstine {

u64 FNVKeyHashFunctor::operator()(const Key& v) const noexcept {
    u64 hash = 1469598103934665603ull;  // FNV offset basis

    for (u8 b : v.bytes()) {
        hash ^= static_cast<u64>(b);
        hash *= 1099511628211ull;  // FNV prime
    }
    return hash;
}

}  // namespace jstine
