#pragma once

#include "Time.hh"
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

enum class KeyspaceType { std, v1 };

Str keyspaceTypeToString(KeyspaceType type);
KeyspaceType keyspaceTypeFromString(const Str& str);

class Config {
   public:
    struct Api {
        u16 port;
        u16 concurrency;
    };

    struct Log {
        log::Level level;
    };

    struct Storage {
        KeyspaceType keyspace;
        std::chrono::seconds reaperInterval;
        std::chrono::seconds defaultExpiration;
    };

    const Api& api() const;
    Api& api();

    const Log& log() const;
    Log& log();

    const Storage& storage() const;
    Storage& storage();

    static Result<Config> load(
        int argc, char** argv, const ConfigFileReader& reader
    );

   private:
    void overrideFields(int argc, char** argv);

    Api m_api;
    Log m_log;
    Storage m_storage;
};

}  // namespace jstine
