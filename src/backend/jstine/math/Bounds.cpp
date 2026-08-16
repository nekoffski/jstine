#include "Bounds.hh"

namespace jstine {

Bounds2i::Bounds2i(const Point2i& corner, const Extent2u& extent)
    : m_min(corner),
      m_max(
          corner +
          Point2i{static_cast<i32>(extent.w), static_cast<i32>(extent.h)}
      ) {}

Bounds2i::Bounds2i(const Extent2u& extent) : Bounds2i(Point2i{0, 0}, extent) {}

Bounds2i::Bounds2i(const Point2i& min, const Point2i& max)
    : m_min(min), m_max(max) {}

Point2i Bounds2i::max() const { return m_max; }
Point2i Bounds2i::min() const { return m_min; }

bool Bounds2i::contains(const Point2i& point) const {
    return point.x >= m_min.x && point.x < m_max.x && point.y >= m_min.y &&
           point.y < m_max.y;
}

u32 Bounds2i::area() const {
    auto extent = this->extent();
    return extent.w * extent.h;
}

Extent2u Bounds2i::extent() const {
    return Extent2u{
        static_cast<u32>(m_max.x - m_min.x), static_cast<u32>(m_max.y - m_min.y)
    };
}

}  // namespace jstine