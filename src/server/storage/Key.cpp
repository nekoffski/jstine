#include "Key.hh"

namespace jstine {

Key::Key(Bytes&& bytes) : m_bytes(std::move(bytes)) {}

Key::Key(const Bytes& bytes) : m_bytes(bytes) {}

const Bytes& Key::bytes() const { return m_bytes; }

bool Key::operator==(const Key& other) const {
    return m_bytes == other.m_bytes;
}

}  // namespace jstine
