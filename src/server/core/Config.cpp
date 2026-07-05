#include "Config.hh"

#include <CLI/CLI.hpp>

#include "core/Log.hh"

namespace jstine {

namespace {

Result<Path> readConfigPath(int argc, char** argv) {
    CLI::App app{"jstine server"};

    Path configPath;
    app.add_option("-c,--config", configPath, "Path to the configuration file")
        ->required();
    app.allow_extras();

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return Error::unexpected(
            ErrorCode::badConfig, "Invalid configuration file path: {}",
            e.what()
        );
    }

    if (not configPath.isFile()) {
        return Error::unexpected(
            ErrorCode::badConfig, "Configuration file does not exist: {}",
            configPath.str()
        );
    }

    return configPath;
}

}  // namespace

const Config::Api& Config::api() const { return m_api; }
const Config::Log& Config::log() const { return m_log; }

Config::Api& Config::api() { return m_api; }
Config::Log& Config::log() { return m_log; }

void logFields(const Config& cfg);

Result<Config> Config::load(
    int argc, char** argv, const ConfigFileReader& reader
) {
    Config cfg;

    auto configPath = readConfigPath(argc, argv);

    if (not configPath) {
        return Error::unexpected(configPath.error());
    }

    if (auto err = reader.read(cfg, configPath.value()); err) {
        return Error::unexpected(err.value());
    }

    cfg.overrideFields(argc, argv);

    logFields(cfg);
    return cfg;
}

void Config::overrideFields(int argc, char** argv) {
    CLI::App app{"jstine server"};
    app.allow_extras();

    app.add_option("--api-port", m_api.port, "API listen port");

    app.add_option("--log-level", m_log.level, "Logging level")
        ->transform(CLI::CheckedTransformer(log::levelMap(), CLI::ignore_case));

    app.parse(argc, argv);
}

void logFields(const Config& cfg) {
    log::info("loaded config:");
    log::info("\tapi.port = {}", cfg.api().port);
    log::info("\tlog.level = {}", log::levelToString(cfg.log().level));
}

}  // namespace jstine
