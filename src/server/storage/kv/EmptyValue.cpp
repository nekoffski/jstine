#include "EmptyValue.hh"

namespace jstine {

std::span<const Byte> EmptyValue::bytes() const { return {}; }

}  // namespace jstine