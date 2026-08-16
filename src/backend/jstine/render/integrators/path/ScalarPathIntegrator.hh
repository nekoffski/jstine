#pragma once

#include "jstine/core/Config.hh"
#include "jstine/render/Integrator.hh"

namespace jstine {

class ScalarPathIntegrator : public Integrator {
   public:
    explicit ScalarPathIntegrator(const Config::Renderer& config);

    Result<void> render(RenderContext& context) override;

   private:
};

}  // namespace jstine
