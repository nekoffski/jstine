#pragma once

#include "jstine/core/Core.hh"

namespace jstine {

struct LinearRgb {
    f32 r;
    f32 g;
    f32 b;
};

struct Xyz {
    f32 x;
    f32 y;
    f32 z;
};

class RgbColorSpace {};

}  // namespace jstine