#pragma once

#include <array>

#include "core/Singleton.hh"
#include "runtime/Signal.hh"

namespace jstine {

class UnixSignalManager : public SignalManager,
                          public UniqueInstance<UnixSignalManager> {
    using Handlers = std::array<Handler, static_cast<u16>(Signal::count)>;

   public:
    Opt<Error> send(Signal signal, Pid pid) override;
    Opt<Error> registerHandler(Signal signal, Handler handler) override;
    void removeHandler(Signal signal) override;

   private:
    static Handlers s_handlers;
};

}  // namespace jstine
