#include "Config.hh"

#include <CLI/CLI.hpp>

#include "Log.hh"

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

std::unordered_map<std::string, ExecutionBackend> executionBackendMap() {
    static std::unordered_map<std::string, ExecutionBackend> map = {
        {"cpu-scalar", ExecutionBackend::cpuScalar},
        {"cpu-simd", ExecutionBackend::cpuSimd},
        {"cuda", ExecutionBackend::cuda},
    };
    return map;
}

std::unordered_map<std::string, IntegratorAlgorithm> integratorAlgorithmMap() {
    static std::unordered_map<std::string, IntegratorAlgorithm> map = {
        {"path", IntegratorAlgorithm::path},
    };
    return map;
}

}  // namespace

const Config::Log& Config::log() const { return m_log; }
Config::Log& Config::log() { return m_log; }

Config::Paths& Config::paths() { return m_paths; }
const Config::Paths& Config::paths() const { return m_paths; }

const Config::Renderer& Config::renderer() const { return m_renderer; }
Config::Renderer& Config::renderer() { return m_renderer; }

void logFields(const Config& cfg);

Result<Config> Config::load(
    int argc, char** argv, const ConfigFileReader& reader
) {
    Config cfg;

    auto configPath = readConfigPath(argc, argv);

    if (not configPath) {
        return Error::unexpected(configPath.error());
    }

    if (auto res = reader.read(cfg, configPath.value()); not res) {
        return Error::unexpected(res.error());
    }

    cfg.overrideFields(argc, argv);

    logFields(cfg);
    return cfg;
}

void Config::overrideFields(int argc, char** argv) {
    CLI::App app{"jstine server"};
    app.allow_extras();

    app.add_option("--log-level", m_log.level, "Logging level")
        ->transform(CLI::CheckedTransformer(log::levelMap(), CLI::ignore_case));

    app.add_option("--output", m_paths.output, "Output path");

    app.add_option("--backend", m_renderer.backend, "Execution backend")
        ->transform(
            CLI::CheckedTransformer(executionBackendMap(), CLI::ignore_case)
        );

    app.add_option(
           "--integrator", m_renderer.integrator, "Integrator algorithm"
    )
        ->transform(
            CLI::CheckedTransformer(integratorAlgorithmMap(), CLI::ignore_case)
        );

    app.parse(argc, argv);
}

void logFields(const Config& cfg) {
    log::info("loaded config:");
    log::info("\tlog.level = {}", log::levelToString(cfg.log().level));
    log::info("\tpaths.output = {}", cfg.paths().output.str());
    log::info("\trenderer.backend = {}", toString(cfg.renderer().backend));
    log::info(
        "\trenderer.integrator = {}", toString(cfg.renderer().integrator)
    );
}

Str toString(ExecutionBackend backend) {
    for (const auto& [key, value] : executionBackendMap()) {
        if (value == backend) {
            return key;
        }
    }
    return "unknown";
}

Str toString(IntegratorAlgorithm integrator) {
    for (const auto& [key, value] : integratorAlgorithmMap()) {
        if (value == integrator) {
            return key;
        }
    }
    return "unknown";
}

ExecutionBackend executionBackendFromString(const Str& str) {
    const auto& map = executionBackendMap();
    if (auto it = map.find(str); it != executionBackendMap().end()) {
        if (it != executionBackendMap().end()) {
            return it->second;
        }
    }
    return ExecutionBackend::cpuScalar;
}

IntegratorAlgorithm integratorAlgorithmFromString(const Str& str) {
    const auto& map = integratorAlgorithmMap();
    if (auto it = map.find(str); it != integratorAlgorithmMap().end()) {
        return it->second;
    }
    return IntegratorAlgorithm::path;
}

}  // namespace jstine
