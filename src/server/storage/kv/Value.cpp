// #include "Value.hh"

// #include "core/Log.hh"

// namespace jstine {

// StrValueBody::StrValueBody(std::span<const Byte> bytes, Allocator* allocator)
//     : m_ptr(nullptr), m_size(bytes.size()), m_allocator(allocator) {
//     m_ptr = static_cast<Byte*>(m_allocator->allocate(m_size));
//     log::expect(m_ptr != nullptr, "Failed to allocate memory");
//     std::copy(bytes.begin(), bytes.end(), m_ptr);
// }

// StrValueBody::~StrValueBody() {
//     if (m_ptr) {
//         m_allocator->free(m_ptr);
//     }
// }

// std::span<Byte> StrValueBody::bytes() { return {m_ptr, m_size}; }
// std::span<const Byte> StrValueBody::bytes() const { return {m_ptr, m_size}; }

// Result<Value> Value::fromBytes(
//     std::span<const Byte> bytes, Allocator& allocator
// ) {
//     return {};
//     // return Value{StrValueBody{bytes, &allocator}};
// }

// // Value::Value(ValueBody body) : m_body(std::move(body)) {}

// }  // namespace jstine