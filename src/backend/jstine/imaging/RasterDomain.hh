#pragma once

#include "jstine/math/Bounds.hh"
#include "jstine/math/Extent.hh"

namespace jstine {

class RasterDomain {
   public:
    explicit RasterDomain(const Extent2u& extent);

    const Bounds2i& displayWindow() const;
    const Bounds2i& dataWindow() const;
    f32 pixelAspectRatio() const;

   private:
    Bounds2i m_displayWindow;
    Bounds2i m_dataWindow;
    f32 m_pixelAspectRatio;
};

}  // namespace jstine