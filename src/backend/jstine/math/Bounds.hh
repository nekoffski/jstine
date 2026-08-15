#pragma once

#include "Extent.hh"
#include "Point.hh"
#include "jstine/core/Core.hh"

namespace jstine {

class Bounds2i {
   public:
    Point2i max() const {}
    Point2i min() const {}

    u32 area() const {}

    Extent2u extent() const {}

   private:
    Point2i m_min;
    Point2i m_max;
};

}  // namespace jstine
