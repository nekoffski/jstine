#pragma once

#include <span>

#include "api/Message.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class RequestDecoder : public NonCopyable, public NonMovable {
   public:
    virtual ~RequestDecoder() = default;

    virtual void feed(std::span<const Byte> bytes) = 0;
    virtual Result<Request> decode() = 0;
};

}  // namespace jstine
