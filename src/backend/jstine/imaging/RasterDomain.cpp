#include "RasterDomain.hh"

namespace jstine {

RasterDomain::RasterDomain(const Extent2u& extent)
    : m_displayWindow(extent), m_dataWindow(extent), m_pixelAspectRatio(1.0f) {}

const Bounds2i& RasterDomain::displayWindow() const { return m_displayWindow; }

const Bounds2i& RasterDomain::dataWindow() const { return m_dataWindow; }

f32 RasterDomain::pixelAspectRatio() const { return m_pixelAspectRatio; }

}  // namespace jstine
