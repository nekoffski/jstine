#include "ValueWrapper.hh"

namespace jstine {

ValueWrapper::ValueWrapper() : m_value(EmptyValue{}) {}

Opt<Error> ValueWrapper::set(
    std::span<const Byte> bytes, Allocator& allocator
) {
    if (auto value = StrValue::parse(&allocator, bytes); not value) {
        return value.error();
    } else {
        m_value = std::move(*value);
    }
    return Error::empty();
}

std::span<const Byte> ValueWrapper::bytes() const {
    return std::visit(
        [](const auto& value) -> std::span<const Byte> {
            return value.bytes();
        },
        m_value
    );
}

const Metadata& ValueWrapper::metadata() const { return m_metadata; }

}  // namespace jstine
