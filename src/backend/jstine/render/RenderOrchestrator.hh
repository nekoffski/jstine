#pragma once

#include "RenderRequest.hh"
#include "RenderSession.hh"
#include "jstine/core/Concepts.hh"
#include "jstine/core/Config.hh"
#include "jstine/core/Core.hh"

namespace jstine {

class RenderOrchestrator : public NonCopyable {
   public:
    static Result<RenderOrchestrator> create(const Config& cfg);

    Result<RenderSession> startSession(const RenderRequest& request);

   private:
};

}  // namespace jstine
