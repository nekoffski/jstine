#include "OS.hh"

namespace jstine {

Str toString(OS os) {
    switch (os) {
        case OS::linux:
            return "Linux";
        case OS::windows:
            return "Windows";
        case OS::darwin:
            return "Darwin";
    }
    return "Unknown";
}

}  // namespace jstine
