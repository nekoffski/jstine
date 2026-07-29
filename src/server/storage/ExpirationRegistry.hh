#pragma once

#include <mutex>
#include <unordered_map>

#include "core/Concepts.hh"
#include "core/Time.hh"
#include "hash/FNVHash.hh"
#include "kv/Key.hh"

namespace jstine {

class ExpirationRegistry : public NonCopyable, public NonMovable {
   public:
    void expiresAfter(const Key& key, std::chrono::seconds duration);
    bool expired(const Key& key, const Clock::time_point& now = Clock::now());

    template <typename Callback>
        requires Callable<
            Callback, bool(const Key& key, const Clock::time_point& expiration)>
    void forEach(Callback&& callback) const {
        std::lock_guard<std::mutex> lock(m_expirationsMutex);
        for (const auto& [key, expiration] : m_expirations) {
            if (not callback(key, expiration)) {
                break;
            }
        }
    }

   private:
    std::unordered_map<Key, Clock::time_point, FNVKeyHashFunctor> m_expirations;
    mutable std::mutex m_expirationsMutex;
};

}  // namespace jstine
