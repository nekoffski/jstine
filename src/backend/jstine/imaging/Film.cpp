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
    : m_spec(spec), m_image(Image<LinearRgb>(spec.domain.dataWindow())) {}

FilmSnapshot Film::snapshot() const {
    std::unique_lock lk{m_imageMutex};
    return FilmSnapshot{
        m_spec.domain, m_spec.colorSpace, Plane<BeautyTag>{m_image}
    };
}

}  // namespace jstine
