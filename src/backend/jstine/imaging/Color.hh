#pragma once

#include "jstine/core/Core.hh"

namespace jstine {

struct LinearRgb {
    f32 r{0.0f};
    f32 g{0.0f};
    f32 b{0.0f};
};

struct Xyz {
    f32 x{0.0f};
    f32 y{0.0f};
    f32 z{0.0f};
};

class RgbColorSpace {
   public:
    enum class Id : u8 { srgb };

    [[nodiscard]] static RgbColorSpace srgb() noexcept {
        return RgbColorSpace{Id::srgb};
    }
    [[nodiscard]] constexpr Id id() const noexcept { return m_id; }

    friend constexpr bool operator==(
        const RgbColorSpace&, const RgbColorSpace&
    ) noexcept = default;

   private:
    explicit constexpr RgbColorSpace(Id id) noexcept : m_id(id) {}

    Id m_id;
};

}  // namespace jstine
