#pragma once

#include <span>

#include "api/Message.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class ResponseEncoder : public NonCopyable, public NonMovable {
   public:
    virtual ~ResponseEncoder() = default;

    virtual Result<u64> encode(
        const Response& response, std::span<Byte> bytes
    ) = 0;
};

}  // namespace jstine
