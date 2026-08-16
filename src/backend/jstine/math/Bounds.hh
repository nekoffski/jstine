#pragma once

#include "Extent.hh"
#include "Point.hh"
#include "jstine/core/Core.hh"

namespace jstine {

class Bounds2i {
   public:
    explicit constexpr Bounds2i(
        const Point2i& corner, const Extent2u& extent
    ) noexcept
        : m_min(corner),
          m_max{
              corner.x + static_cast<i32>(extent.w),
              corner.y + static_cast<i32>(extent.h)
          } {}

    explicit constexpr Bounds2i(const Extent2u& extent) noexcept
        : Bounds2i(Point2i{0, 0}, extent) {}

    explicit constexpr Bounds2i(const Point2i& min, const Point2i& max) noexcept
        : m_min(min), m_max(max) {}

    [[nodiscard]] constexpr Point2i max() const noexcept { return m_max; }
    [[nodiscard]] constexpr Point2i min() const noexcept { return m_min; }

    [[nodiscard]] constexpr bool contains(const Point2i& point) const noexcept {
        return point.x >= m_min.x && point.x < m_max.x && point.y >= m_min.y &&
               point.y < m_max.y;
    }

    [[nodiscard]] constexpr u32 area() const noexcept {
        const auto size = extent();
        return size.w * size.h;
    }

    [[nodiscard]] constexpr Extent2u extent() const noexcept {
        return {
            static_cast<u32>(m_max.x - m_min.x),
            static_cast<u32>(m_max.y - m_min.y)
        };
    }

   private:
    Point2i m_min;
    Point2i m_max;
};

}  // namespace jstine
