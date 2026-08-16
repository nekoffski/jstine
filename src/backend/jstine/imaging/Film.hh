#pragma once

#include <mutex>

#include "Color.hh"
#include "Plane.hh"
#include "RasterDomain.hh"
#include "jstine/core/Concepts.hh"
#include "jstine/core/Error.hh"

namespace jstine {

struct FilmSpec {
    RasterDomain domain;
    RgbColorSpace colorSpace;
};

class Film;

class FilmSnapshot final : public NonCopyable {
    friend class Film;

   public:
    const Plane<BeautyTag>& beauty() const;
    const RasterDomain& rasterDomain() const;
    const RgbColorSpace& colorSpace() const;

   private:
    explicit FilmSnapshot(
        const RasterDomain& domain, const RgbColorSpace& colorSpace,
        Plane<BeautyTag> beauty
    );

    RasterDomain m_domain;
    RgbColorSpace m_colorSpace;
    Plane<BeautyTag> m_beauty;
};

class Film final : public NonCopyable {
    struct PixelAccumulator {
        LinearRgb rgbSum;
        f32 weightSum;
    };

   public:
    explicit Film(const FilmSpec& spec);

    void addRgbSample(const Point2i& pixel, const LinearRgb& rgb, f32 weight);

    FilmSnapshot snapshot() const;
    const Bounds2i& pixelBounds() const;

   private:
    FilmSpec m_spec;
    Image<PixelAccumulator> m_image;
    mutable std::mutex m_imageMutex;
};

}  // namespace jstine
