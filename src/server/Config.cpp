#include "Config.hh"

namespace jstine {

const Config::Api& Config::api() const { return m_api; }

Config::Api& Config::api() { return m_api; }

}  // namespace jstine
