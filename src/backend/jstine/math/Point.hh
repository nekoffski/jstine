#pragma once

#include "jstine/core/Core.hh"

namespace jstine {

struct Point2i {
    i32 x;
    i32 y;

    Point2i operator+(const Point2i& other) const;
};

}  // namespace jstine
