#pragma once

#include <cmath>

#include "jstine/core/Core.hh"

namespace jstine {

struct Vector3f {
    f32 x{0.0f};
    f32 y{0.0f};
    f32 z{0.0f};

    constexpr Vector3f& operator+=(const Vector3f& other) noexcept {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    constexpr Vector3f& operator-=(const Vector3f& other) noexcept {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    constexpr Vector3f& operator*=(f32 scalar) noexcept {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vector3f& operator/=(f32 scalar) noexcept {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    friend constexpr bool operator==(
        const Vector3f&, const Vector3f&
    ) noexcept = default;
};

[[nodiscard]] constexpr Vector3f operator+(
    Vector3f lhs, const Vector3f& rhs
) noexcept {
    return lhs += rhs;
}

[[nodiscard]] constexpr Vector3f operator-(
    Vector3f lhs, const Vector3f& rhs
) noexcept {
    return lhs -= rhs;
}

[[nodiscard]] constexpr Vector3f operator-(const Vector3f& vector) noexcept {
    return {-vector.x, -vector.y, -vector.z};
}

[[nodiscard]] constexpr Vector3f operator*(
    Vector3f vector, f32 scalar
) noexcept {
    return vector *= scalar;
}

[[nodiscard]] constexpr Vector3f operator*(
    f32 scalar, Vector3f vector
) noexcept {
    return vector *= scalar;
}

[[nodiscard]] constexpr Vector3f operator/(
    Vector3f vector, f32 scalar
) noexcept {
    return vector /= scalar;
}

[[nodiscard]] constexpr f32 dot(
    const Vector3f& lhs, const Vector3f& rhs
) noexcept {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] constexpr Vector3f cross(
    const Vector3f& lhs, const Vector3f& rhs
) noexcept {
    return {
        lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x
    };
}

[[nodiscard]] constexpr f32 lengthSquared(const Vector3f& vector) noexcept {
    return dot(vector, vector);
}

[[nodiscard]] inline f32 length(const Vector3f& vector) noexcept {
    return std::sqrt(lengthSquared(vector));
}

[[nodiscard]] inline Vector3f normalize(const Vector3f& vector) noexcept {
    return vector / length(vector);
}

}  // namespace jstine
