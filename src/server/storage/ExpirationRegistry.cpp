#include "ExpirationRegistry.hh"

namespace jstine {

void ExpirationRegistry::expiresAfter(
    const Key& key, std::chrono::seconds duration
) {
    m_expirations[key] = Clock::now() + duration;
}

bool ExpirationRegistry::expired(
    const Key& key, const Clock::time_point& now
) const {
    if (auto it = m_expirations.find(key); it != m_expirations.end()) {
        return now > it->second;
    }
    return false;
}

}  // namespace jstine
