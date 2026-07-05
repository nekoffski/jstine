#pragma once

#include "Core.hh"

#ifdef _WIN32
#define JSTINE_PLATFORM_WINDOWS
#elif __APPLE__
#define JSTINE_PLATFORM_DARWIN
#else
#define JSTINE_PLATFORM_LINUX
#endif

namespace jstine {

enum class OS {
    linux,
    windows,
    darwin,
};

inline constexpr OS detectOs() {
#ifdef JSTINE_PLATFORM_WINDOWS
    return OS::windows;
#elif defined(JSTINE_PLATFORM_DARWIN)
    return OS::darwin;
#else
    return OS::linux;
#endif
}

Str toString(OS os);

}  // namespace jstine
