#include "Server.hh"
#include "adapters/TomlConfigReader.hh"
#include "core/Config.hh"
#include "core/Log.hh"

using namespace jstine;

int main(int argc, char** argv) {
    TomlConfigReader reader;
    auto config = Config::load(argc, argv, reader);
    log::expect(config);
    log::setLogLevel(config->log().level);
    logFields(config.value());

    return 0;
}
