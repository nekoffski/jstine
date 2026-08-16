#include "RenderOrchestrator.hh"

#include "integrators/path/ScalarPathIntegrator.hh"
#include "jstine/core/OS.hh"

namespace jstine {

namespace {

Opt<Error> validateRenderer(const Config::Renderer& cfg) {
    const auto os = detectOs();

    if (os == OS::darwin && cfg.backend == ExecutionBackend::cuda) {
        return Error{
            ErrorCode::badConfig, "CUDA backend is not supported on macOS"
        };
    }
    return {};
}

}  // namespace

RenderOrchestrator::RenderOrchestrator(const Config& cfg) : m_cfg(cfg) {}

Result<RenderOrchestrator> RenderOrchestrator::create(const Config& cfg) {
    if (auto err = validateRenderer(cfg.renderer())) {
        return Error::unexpected(err.value());
    }
    return RenderOrchestrator{cfg};
}

Result<RenderSession> RenderOrchestrator::startSession(
    const RenderRequest& request
) {
    auto integrator = createIntegrator(m_cfg.renderer());
    if (not integrator) {
        return Error::unexpected(integrator.error());
    }
    return RenderSession(std::move(integrator.value()));
}

Result<std::unique_ptr<Integrator>> RenderOrchestrator::createIntegrator(
    const Config::Renderer& cfg
) {
    if (cfg.backend == ExecutionBackend::cpuScalar &&
        cfg.integrator == IntegratorAlgorithm::path) {
        return std::make_unique<ScalarPathIntegrator>(cfg);
    }
    return Error::unexpected(ErrorCode::badConfig, "Unsupported integrator");
}
}  // namespace jstine
