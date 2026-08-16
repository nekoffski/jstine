#include "jstine/adapters/PngFilmSnapshotWriter.hh"
#include "jstine/core/Config.hh"
#include "jstine/core/Log.hh"
#include "jstine/core/TomlConfigReader.hh"
#include "jstine/core/Unwrap.hh"
#include "jstine/math/Ray.hh"
#include "jstine/math/Vector.hh"
#include "jstine/render/RenderOrchestrator.hh"

using namespace jstine;

int main(int argc, char** argv) {
    log::init();

    auto cfg = unwrap(Config::load(argc, argv, TomlConfigReader{}));
    auto orchestrator = unwrap(RenderOrchestrator::create(cfg));

    RenderRequest request{
        .film = {
            .domain = RasterDomain{Extent2u{480, 480}},
            .colorSpace = RgbColorSpace::srgb()
        }
    };
    auto session = unwrap(orchestrator.startSession(request));

    if (auto res = session.wait(); not res) {
        log::error(
            "Error occurred while waiting for render session: {}",
            res.error().message()
        );
        return -1;
    }

    auto snapshot = session.snapshot();
    log::expect(PngFilmSnapshotWriter{}.write(snapshot, cfg.paths().output));

    return 0;
}
