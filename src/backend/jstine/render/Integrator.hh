#pragma once

#include "RenderContext.hh"
#include "jstine/core/Concepts.hh"
#include "jstine/core/Error.hh"

namespace jstine {

class Integrator : public NonCopyable, public NonMovable {
   public:
    virtual ~Integrator() = default;
    virtual Result<void> render(RenderContext& ctx) = 0;
};

}  // namespace jstine