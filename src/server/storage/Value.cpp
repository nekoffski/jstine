#include "Value.hh"

namespace jstine {

StrValueBody::StrValueBody(const Bytes& bytes) : m_bytes(bytes) {}

Bytes& StrValueBody::bytes() { return m_bytes; }
const Bytes& StrValueBody::bytes() const { return m_bytes; }

Result<Value> Value::fromBytes(const Bytes& bytes) {
    return Value{StrValueBody{bytes}};
}

Value::Value(ValueBody body) : m_body(std::move(body)) {}

const Bytes& Value::bytes() const {
    return std::visit(
        [](const auto& value) -> const Bytes& { return value.bytes(); }, m_body
    );
}

const Metadata& Value::metadata() const { return m_metadata; }

}  // namespace jstine