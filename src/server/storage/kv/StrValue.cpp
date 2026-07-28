#include "StrValue.hh"

namespace jstine {

std::span<const Byte> StrValue::bytes() const {
    return std::span{m_bytes, m_size};
}

Result<StrValue> StrValue::parse(
    Allocator* allocator, std::span<const Byte> input
) {
    auto ptr = static_cast<Byte*>(allocator->allocate(input.size()));

    if (not ptr) {
        return Error::unexpected(
            ErrorCode::allocatorFailure, "Failed to allocate memory"
        );
    }

    std::memcpy(ptr, input.data(), input.size());
    return StrValue{allocator, input.size(), ptr};
}

StrValue::StrValue(StrValue&& other) noexcept
    : m_allocator(std::exchange(other.m_allocator, nullptr)),
      m_size(std::exchange(other.m_size, 0)),
      m_bytes(std::exchange(other.m_bytes, nullptr)) {}

StrValue& StrValue::operator=(StrValue&& other) noexcept {
    if (this != &other) {
        if (m_bytes) {
            m_allocator->free(m_bytes);
        }
        m_allocator = std::exchange(other.m_allocator, nullptr);
        m_size = std::exchange(other.m_size, 0);
        m_bytes = std::exchange(other.m_bytes, nullptr);
    }
    return *this;
}

StrValue::StrValue(Allocator* allocator, u64 size, Byte* bytes)
    : m_allocator(allocator), m_size(size), m_bytes(bytes) {}

StrValue::~StrValue() {
    if (m_bytes) {
        m_allocator->free(m_bytes);
    }
}

}  // namespace jstine
