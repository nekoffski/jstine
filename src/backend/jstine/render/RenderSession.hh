#pragma once

#include "jstine/core/Concepts.hh"
#include "jstine/core/Core.hh"
#include "jstine/imaging/Film.hh"

namespace jstine {

class RenderSession : public NonCopyable {
   public:
    bool finished() const { return true; }

    Result<FilmSnapshot> snapshot() {}

   private:
};

}  // namespace jstine
