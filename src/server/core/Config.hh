#pragma once

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "core/FileSystem.hh"
#include "core/Log.hh"

namespace jstine {

class Config;

class ConfigFileReader : public NonCopyable, public NonMovable {
   public:
    virtual ~ConfigFileReader() = default;

    virtual Opt<Error> read(Config& config, const Path& path) const = 0;
};

class Config {
   public:
    struct Api {
        u16 port;
        u16 concurrency;
    };

    struct Log {
        log::Level level;
    };

    const Api& api() const;
    Api& api();

    const Log& log() const;
    Log& log();

    static Result<Config> load(
        int argc, char** argv, const ConfigFileReader& reader
    );

   private:
    void overrideFields(int argc, char** argv);

    Api m_api;
    Log m_log;
};

}  // namespace jstine
