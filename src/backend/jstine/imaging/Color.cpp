#include "Color.hh"

namespace jstine {

RgbColorSpace RgbColorSpace::srgb() noexcept { return RgbColorSpace{Id::srgb}; }

constexpr RgbColorSpace::Id RgbColorSpace::id() const noexcept { return m_id; }

constexpr RgbColorSpace::RgbColorSpace(Id id) noexcept : m_id(id) {}

}  // namespace jstine
