#pragma once

#include "Extent.hh"
#include "Point.hh"
#include "jstine/core/Core.hh"

namespace jstine {

class Bounds2i {
   public:
    explicit Bounds2i(const Point2i& corner, const Extent2u& extent);
    explicit Bounds2i(const Extent2u& extent);
    explicit Bounds2i(const Point2i& min, const Point2i& max);

    Point2i max() const;
    Point2i min() const;

    bool contains(const Point2i& point) const;

    u32 area() const;

    Extent2u extent() const;

   private:
    Point2i m_min;
    Point2i m_max;
};

}  // namespace jstine
