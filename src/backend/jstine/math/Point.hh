#pragma once

#include "Vector.hh"
#include "jstine/core/Core.hh"

namespace jstine {

struct Point2i {
    i32 x{0};
    i32 y{0};

    friend constexpr bool operator==(const Point2i&, const Point2i&) noexcept =
        default;
};

struct Point3f {
    f32 x{0.0f};
    f32 y{0.0f};
    f32 z{0.0f};

    friend constexpr bool operator==(const Point3f&, const Point3f&) noexcept =
        default;
};

[[nodiscard]] constexpr Point3f operator+(
    const Point3f& point, const Vector3f& vector
) noexcept {
    return {point.x + vector.x, point.y + vector.y, point.z + vector.z};
}

[[nodiscard]] constexpr Point3f operator+(
    const Vector3f& vector, const Point3f& point
) noexcept {
    return point + vector;
}

[[nodiscard]] constexpr Point3f operator-(
    const Point3f& point, const Vector3f& vector
) noexcept {
    return {point.x - vector.x, point.y - vector.y, point.z - vector.z};
}

[[nodiscard]] constexpr Vector3f operator-(
    const Point3f& lhs, const Point3f& rhs
) noexcept {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

}  // namespace jstine
