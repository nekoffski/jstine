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

class Config {
   public:
    struct Log {
        log::Level level;
    };

    const Log& log() const;
    Log& log();

    static Result<Config> load(
        int argc, char** argv, const ConfigFileReader& reader
    );

   private:
    void overrideFields(int argc, char** argv);

    Log m_log;
};

}  // namespace jstine
