#pragma once

#include "Color.hh"
#include "Image.hh"

namespace jstine {

template <typename Tag>
class Plane {
    using Value = typename Tag::Value;

   public:
    explicit Plane(Image<Value> image) : m_image(std::move(image)) {}

    Bounds2i bounds() const noexcept { return m_image.bounds(); }

    const Value& operator()(Point2i point) const noexcept {
        return m_image(point);
    }

    constexpr u8 channels() const noexcept { return Tag::channels; }
    const Value& at(Point2i point) const noexcept { return m_image.at(point); }
    std::span<const Value> row(i32 y) const noexcept { return m_image.row(y); }
    std::span<const Value> pixels() const noexcept { return m_image.pixels(); }

   private:
    Image<Value> m_image;
};

struct BeautyTag {
    static constexpr u8 channels = 3;
    using Value = LinearRgb;
};

struct AlbedoTag {
    static constexpr u8 channels = 3;
    using Value = LinearRgb;
};

}  // namespace jstine
