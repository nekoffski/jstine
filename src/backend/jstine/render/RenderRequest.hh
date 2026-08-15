#pragma once

#include "jstine/core/Concepts.hh"
#include "jstine/core/Config.hh"
#include "jstine/core/Core.hh"
#include "jstine/imaging/Film.hh"

namespace jstine {

struct RenderRequest {
    FilmSpec film;
};

}  // namespace jstine