#pragma once

#include <unordered_map>

#include "Key.hh"
#include "core/Time.hh"
#include "hash/FNV.hh"

namespace jstine {

class ExpirationRegistry : public NonCopyable, public NonMovable {
   public:
    void expiresAfter(const Key& key, std::chrono::seconds duration);

    bool expired(
        const Key& key, const Clock::time_point& now = Clock::now()
    ) const;

   private:
    std::unordered_map<Key, Clock::time_point, FNVKeyHashFunctor> m_expirations;
};

}  // namespace jstine
