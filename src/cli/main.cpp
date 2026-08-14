#include "jstine/core/Config.hh"
#include "jstine/core/Log.hh"
#include "jstine/core/TomlConfigReader.hh"
#include "jstine/core/Unwrap.hh"
#include "jstine/render/RenderOrchestrator.hh"

using namespace jstine;

int main(int argc, char** argv) {
    log::init();
    auto cfg = unwrap(Config::load(argc, argv, TomlConfigReader{}));
    auto orchestrator = unwrap(RenderOrchestrator::create(cfg));

    RenderRequest request;
    auto session = unwrap(orchestrator.startSession(request));

    while (not session.finished()) {
        break;
    }

    return 0;
}
