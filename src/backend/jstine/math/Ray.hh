#pragma once

#include "Point.hh"
#include "Vector.hh"

namespace jstine {

struct Ray {
    Point3f origin;
    Vector3f direction;
    f32 time{0.0f};

    [[nodiscard]] constexpr Point3f at(f32 t) const noexcept {
        return origin + direction * t;
    }
};

}  // namespace jstine
