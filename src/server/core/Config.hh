#pragma once

#include "core/Concepts.hh"
#include "core/Core.hh"

namespace jstine {

class Config {
   public:
    struct Api {
        u16 port;
    };

    const Api& api() const;
    Api& api();

    static Config load(int argc, char** argv);

   private:
    Api m_api;
};

}  // namespace jstine
