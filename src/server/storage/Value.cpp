#include "Value.hh"

#include "core/Log.hh"

namespace jstine {

StrValueBody::StrValueBody(std::span<const Byte> bytes, Allocator* allocator)
    : m_ptr(nullptr), m_size(bytes.size()), m_allocator(allocator) {
    m_ptr = static_cast<Byte*>(m_allocator->allocate(m_size));
    log::expect(m_ptr != nullptr, "Failed to allocate memory");
    std::copy(bytes.begin(), bytes.end(), m_ptr);
}

StrValueBody::~StrValueBody() {
    if (m_ptr) {
        m_allocator->free(m_ptr);
    }
}

StrValueBody::StrValueBody(StrValueBody&& other) noexcept
    : m_ptr(other.m_ptr), m_size(other.m_size), m_allocator(other.m_allocator) {
    other.m_ptr = nullptr;
    other.m_size = 0;
    other.m_allocator = nullptr;
}

StrValueBody& StrValueBody::operator=(StrValueBody&& other) noexcept {
    if (this != &other) {
        if (m_ptr) {
            m_allocator->free(m_ptr);
        }
        m_ptr = other.m_ptr;
        m_size = other.m_size;
        m_allocator = other.m_allocator;
        other.m_ptr = nullptr;
        other.m_size = 0;
        other.m_allocator = nullptr;
    }
    return *this;
}

StrValueBody& StrValueBody::operator=(const StrValueBody& other) {
    if (this != &other) {
        if (m_ptr) {
            m_allocator->free(m_ptr);
        }
        m_size = other.m_size;
        m_allocator = other.m_allocator;

        m_ptr = static_cast<Byte*>(m_allocator->allocate(m_size));
        log::expect(m_ptr != nullptr, "Failed to allocate memory");
        std::copy(other.m_ptr, other.m_ptr + m_size, m_ptr);
    }
    return *this;
}

std::span<Byte> StrValueBody::bytes() { return {m_ptr, m_size}; }
std::span<const Byte> StrValueBody::bytes() const { return {m_ptr, m_size}; }

Result<Value> Value::fromBytes(
    std::span<const Byte> bytes, Allocator& allocator
) {
    return Value{StrValueBody{bytes, &allocator}};
}

Value::Value(ValueBody body) : m_body(std::move(body)) {}

std::span<const Byte> Value::bytes() const {
    return std::visit(
        [](const auto& value) -> std::span<const Byte> {
            return value.bytes();
        },
        m_body
    );
}

const Metadata& Value::metadata() const { return m_metadata; }

}  // namespace jstine