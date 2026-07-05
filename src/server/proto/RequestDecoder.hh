#pragma once

#include "api/Message.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class RequestDecoder : public NonCopyable, public NonMovable {
   public:
    virtual ~RequestDecoder() = default;

    virtual Result<Request> decode() = 0;
};

}  // namespace jstine
