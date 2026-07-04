#pragma once

#include "core/Concepts.hh"
#include "core/Core.hh"

namespace jstine {

class MessageHandler : public NonCopyable, public NonMovable {
   public:
    Str onRequest(const Str& request);

   private:
};

}  // namespace jstine
