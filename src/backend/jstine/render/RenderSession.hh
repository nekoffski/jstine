#pragma once

#include "jstine/core/Concepts.hh"
#include "jstine/core/Core.hh"

namespace jstine {

class RenderSession : public NonCopyable {
   public:
    bool finished() const { return true; }

   private:
};

}  // namespace jstine
