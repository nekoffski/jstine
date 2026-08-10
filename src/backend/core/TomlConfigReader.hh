#pragma once

#include "core/Config.hh"

namespace jstine {

class TomlConfigReader : public ConfigFileReader {
   public:
    Opt<Error> read(Config& config, const Path& path) const override;
};

}  // namespace jstine
