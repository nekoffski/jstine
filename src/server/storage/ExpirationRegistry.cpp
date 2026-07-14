#include "ExpirationRegistry.hh"

namespace jstine {

void ExpirationRegistry::expiresAfter(
    const Key& key, std::chrono::seconds duration
) {
    m_expirations[key] = Clock::now() + duration;
}

bool ExpirationRegistry::expired(const Key& key, const Clock::time_point& now) {
    if (auto it = m_expirations.find(key); it != m_expirations.end()) {
        if (now > it->second) {
            m_expirations.erase(it);
            return true;
        }
    }
    return false;
}

}  // namespace jstine
