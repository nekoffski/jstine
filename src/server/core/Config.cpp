#include "Config.hh"

#include <CLI/CLI.hpp>

namespace jstine {

const Config::Api& Config::api() const { return m_api; }

Config::Api& Config::api() { return m_api; }

Config Config::load(int argc, char** argv) {
    Config cfg;
    CLI::App app{"jstine server"};

    app.add_option("--api-port", cfg.m_api.port, "API listen port")->required();

    app.parse(argc, argv);
    return cfg;
}

}  // namespace jstine
