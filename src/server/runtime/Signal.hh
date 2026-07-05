#pragma once

#include "Process.hh"
#include "core/Concepts.hh"
#include "core/Error.hh"

namespace jstine {

enum class Signal { terminate, interrupt, hangup, kill, quit, count };

Str signalToString(Signal signal);

class SignalManager : public NonCopyable, public NonMovable {
   public:
    using Handler = void (*)();

    virtual ~SignalManager() = default;

    virtual Opt<Error> send(Signal signal, Pid pid) = 0;
    virtual Opt<Error> registerHandler(Signal signal, Handler handler) = 0;
    virtual void removeHandler(Signal signal) = 0;
};

}  // namespace jstine
