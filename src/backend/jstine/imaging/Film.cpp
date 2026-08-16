#include "Film.hh"

namespace jstine {

FilmSnapshot::FilmSnapshot(
    const RasterDomain& domain, const RgbColorSpace& colorSpace,
    Plane<BeautyTag> beauty
)
    : m_domain(domain), m_colorSpace(colorSpace), m_beauty(std::move(beauty)) {}

const Plane<BeautyTag>& FilmSnapshot::beauty() const { return m_beauty; }

const RasterDomain& FilmSnapshot::rasterDomain() const { return m_domain; }

const RgbColorSpace& FilmSnapshot::colorSpace() const { return m_colorSpace; }

Film::Film(const FilmSpec& spec)
    : m_spec(spec),
      m_image(Image<PixelAccumulator>(spec.domain.dataWindow())) {}

void Film::addRgbSample(
    const Point2i& pixel, const LinearRgb& rgb, f32 weight
) {
    std::unique_lock lk{m_imageMutex};
    auto& accumulator = m_image(pixel);
    accumulator.rgbSum += rgb * weight;
    accumulator.weightSum += weight;
}

FilmSnapshot Film::snapshot() const {
    Image<LinearRgb> snapshotImage{m_image.bounds()};

    {
        std::unique_lock lk{m_imageMutex};
        const auto& pixels = m_image.pixels();

        for (u64 i = 0; i < pixels.size(); ++i) {
            const auto& [rgbSum, weightSum] = pixels[i];
            if (weightSum > 0.0f) {
                snapshotImage.pixels()[i] = rgbSum * (1.0f / weightSum);
            }
        }
    }

    return FilmSnapshot{
        m_spec.domain, m_spec.colorSpace,
        Plane<BeautyTag>{std::move(snapshotImage)}
    };
}

const Bounds2i& Film::pixelBounds() const { return m_spec.domain.dataWindow(); }

}  // namespace jstine
