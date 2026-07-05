#pragma once

#include <array>
#include <cstring>
#include <span>

#include "core/Core.hh"
#include "core/Error.hh"
#include "core/Log.hh"

namespace jstine {

enum class Protocol : u32 { rsp = 1, jfp = 2 };

Str protocolToStr(Protocol protocol);

struct ProtocolHeader {
    static constexpr u64 magicRequest = 0xDEADBEEFu;
    static constexpr u64 magicResponse = 0xBEEFDEADu;

    Protocol protocol;
    u32 magic;
    std::array<Byte, 8> padding;

    bool protocolValid() const;
};

}  // namespace jstine