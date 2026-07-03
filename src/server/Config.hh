#pragma once

#include "core/Concepts.hh"
#include "core/Core.hh"

namespace jstine {

class Config : public NonCopyable, public NonMovable {
   public:
    struct Api {};

    const Api& api() const;
    Api& api();

   private:
    Api m_api;
};

}  // namespace jstine
