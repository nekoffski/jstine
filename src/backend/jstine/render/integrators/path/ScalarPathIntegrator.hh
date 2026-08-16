#pragma once

#include "jstine/core/Config.hh"
#include "jstine/render/Integrator.hh"

namespace jstine {

class ScalarPathIntegrator : public Integrator {
   public:
    explicit ScalarPathIntegrator(const Config::Renderer& config) {}

   private:
};

}  // namespace jstine
