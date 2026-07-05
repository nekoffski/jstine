#include "Server.hh"
#include "adapters/TomlConfigReader.hh"
#include "adapters/platform/Platform.hh"
#include "core/Config.hh"
#include "core/Log.hh"
#include "core/OS.hh"
#include "core/Scope.hh"

using namespace jstine;

int main(int argc, char** argv) {
    log::init(log::LoggerOptions{
        .enableColors = true,
        .formatPattern = "[%Y-%m-%d %T] [Th: %t] %-5l [jstined]: %v",
    });

    log::info("jstine server ({}) starting..", toString(detectOs()));

    {
        TomlConfigReader reader;
        auto config = Config::load(argc, argv, reader);
        log::expect(config);
        log::setLogLevel(config->log().level);

        auto signalManager = createSignalManager();

        ServerContext ctx{
            .signals = *signalManager,
            .config = *config,
        };

        if (auto err = Server{ctx}.run(); err) {
            auto code = fmt::underlying(err->code());
            log::error(
                "Server finished with error ({}): {}", code, err->message()
            );
            return code;
        }
    }

    log::info("jstine server finished gracefully, cya!");
    return 0;
}
