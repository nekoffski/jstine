#pragma once

#include "Core.hh"

namespace jstine {

enum class ErrorCode : u32 {
    noError = 0,
    badConfig = 1,
    fileSystemError = 2,
    invalidArgument = 3,
    imageWriteError = 4,
};

}  // namespace jstine
