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
const Config::Storage& Config::storage() const { return m_storage; }

Config::Api& Config::api() { return m_api; }
Config::Log& Config::log() { return m_log; }
Config::Storage& Config::storage() { return m_storage; }

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
    app.add_option("--api-concurrency", m_api.concurrency, "API concurrency");

    app.add_option("--log-level", m_log.level, "Logging level")
        ->transform(CLI::CheckedTransformer(log::levelMap(), CLI::ignore_case));

    app.add_option(
           "--storage-keyspace-type", m_storage.keyspace, "Keyspace type"
    )
        ->transform(
            CLI::CheckedTransformer(
                std::map<std::string, KeyspaceType>{
                    {"std", KeyspaceType::std},
                    {"v1", KeyspaceType::v1},
                },
                CLI::ignore_case
            )
        );

    app.add_option(
        "--storage-reaper-interval", m_storage.reaperInterval,
        "Storage reaper interval in seconds"
    );

    app.add_option(
        "--storage-default-expiration", m_storage.defaultExpiration,
        "Storage default expiration in seconds"
    );

    app.parse(argc, argv);
}

void logFields(const Config& cfg) {
    log::info("loaded config:");
    log::info("\tapi.port = {}", cfg.api().port);
    log::info("\tapi.concurrency = {}", cfg.api().concurrency);
    log::info("\tlog.level = {}", log::levelToString(cfg.log().level));
    log::info(
        "\tstorage.keyspace = {}", keyspaceTypeToString(cfg.storage().keyspace)
    );
    log::info(
        "\tstorage.reaperInterval = {} seconds",
        cfg.storage().reaperInterval.count()
    );
    log::info(
        "\tstorage.defaultExpiration = {} seconds",
        cfg.storage().defaultExpiration.count()
    );
}

Str keyspaceTypeToString(KeyspaceType type) {
    switch (type) {
        case KeyspaceType::std:
            return "std";
        case KeyspaceType::v1:
            return "v1";
    }
    return "";
}

KeyspaceType keyspaceTypeFromString(const Str& str) {
    if (str == "std") {
        return KeyspaceType::std;
    } else if (str == "v1") {
        return KeyspaceType::v1;
    } else {
        log::warn("Unknown keyspace type '{}', defaulting to 'std'", str);
        return KeyspaceType::std;
    }
}

}  // namespace jstine
