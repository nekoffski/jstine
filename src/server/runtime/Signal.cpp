#include "Signal.hh"

namespace jstine {

Str signalToString(Signal signal) {
    switch (signal) {
        case Signal::terminate:
            return "SIGTERM";
        case Signal::interrupt:
            return "SIGINT";
        case Signal::hangup:
            return "SIGHUP";
        case Signal::kill:
            return "SIGKILL";
        case Signal::quit:
            return "SIGQUIT";
        default:
            return "UNKNOWN";
    }
}

}  // namespace jstine
