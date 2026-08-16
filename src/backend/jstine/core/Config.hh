#pragma once

#include "Concepts.hh"
#include "Core.hh"
#include "Error.hh"
#include "FileSystem.hh"
#include "Log.hh"
#include "Time.hh"

namespace jstine {

class Config;

class ConfigFileReader : public NonCopyable, public NonMovable {
   public:
    virtual ~ConfigFileReader() = default;

    virtual Opt<Error> read(Config& config, const Path& path) const = 0;
};

enum class ExecutionBackend {
    cpuScalar,
    cpuSimd,
    cuda,
};

enum class IntegratorAlgorithm {
    path,
};

Str toString(ExecutionBackend backend);
Str toString(IntegratorAlgorithm integrator);

ExecutionBackend executionBackendFromString(const Str& str);
IntegratorAlgorithm integratorAlgorithmFromString(const Str& str);

class Config {
   public:
    struct Log {
        log::Level level;
    };

    struct Paths {
        Path output;
    };

    struct Renderer {
        ExecutionBackend backend;
        IntegratorAlgorithm integrator;
    };

    const Log& log() const;
    Log& log();

    Paths& paths();
    const Paths& paths() const;

    const Renderer& renderer() const;
    Renderer& renderer();

    static Result<Config> load(
        int argc, char** argv, const ConfigFileReader& reader
    );

   private:
    void overrideFields(int argc, char** argv);

    Log m_log;
    Paths m_paths;
    Renderer m_renderer;
};

}  // namespace jstine
