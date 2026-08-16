#pragma once

#include "Config.hh"

namespace jstine {

class TomlConfigReader : public ConfigFileReader {
   public:
    Result<void> read(Config& config, const Path& path) const override;
};

}  // namespace jstine
