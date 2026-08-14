#include "RenderOrchestrator.hh"

namespace jstine {

Result<RenderOrchestrator> RenderOrchestrator::create(const Config& cfg) {
    return Result<RenderOrchestrator>();
}

Result<RenderSession> RenderOrchestrator::startSession(
    const RenderRequest& request
) {
    return Result<RenderSession>();
}

}  // namespace jstine
