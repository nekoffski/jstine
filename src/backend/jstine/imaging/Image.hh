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

    Bounds2i bounds() const noexcept { return m_bounds; }

    Extent2u extent() const noexcept { return m_bounds.extent(); }

    Pixel& operator()(Point2i point) noexcept {
        const auto index = point.x - m_bounds.min().x +
                           (point.y - m_bounds.min().y) * extent().w;
        return m_pixels[index];
    }

    const Pixel& operator()(Point2i point) const noexcept {
        const auto index = point.x - m_bounds.min().x +
                           (point.y - m_bounds.min().y) * extent().w;
        return m_pixels[index];
    }

    std::span<Pixel> row(i32 y) noexcept {
        return std::span{
            m_pixels.data() + (y - m_bounds.min().y) * extent().w, extent().w
        };
    }

    std::span<const Pixel> row(i32 y) const noexcept {
        return std::span{
            m_pixels.data() + (y - m_bounds.min().y) * extent().w, extent().w
        };
    }

    std::span<const Pixel> pixels() const noexcept { return m_pixels; }
    std::span<Pixel> pixels() noexcept { return m_pixels; }

   private:
    Bounds2i m_bounds;
    std::vector<Pixel> m_pixels;
};

}  // namespace jstine
