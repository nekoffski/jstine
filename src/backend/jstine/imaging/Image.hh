#pragma once

#include <span>
#include <vector>

#include "jstine/core/Core.hh"
#include "jstine/math/Bounds.hh"
#include "jstine/math/Extent.hh"

namespace jstine {

template <typename Pixel>
class Image {
   public:
    explicit Image(const Bounds2i& bounds, const Pixel& def = Pixel{})
        : m_bounds(bounds), m_pixels(bounds.area(), def) {}

    Bounds2i bounds() const noexcept {}

    Extent2u extent() const noexcept {}

    Pixel& operator()(Point2i point) noexcept {}

    const Pixel& operator()(Point2i point) const noexcept {}

    const Pixel& at(Point2i point) const noexcept {}

    std::span<Pixel> row(i32 y) noexcept {}

    std::span<const Pixel> row(i32 y) const noexcept {}

    std::span<const Pixel> pixels() const noexcept { return m_pixels; }

   private:
    Bounds2i m_bounds;
    std::vector<Pixel> m_pixels;
};

}  // namespace jstine
