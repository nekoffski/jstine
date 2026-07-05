#include "Signal.hh"

#include <signal.h>
#include <unistd.h>

#include "core/ErrorCode.hh"

namespace jstine {

UnixSignalManager::Handlers UnixSignalManager::s_handlers;

Result<i32> toUnixSignal(Signal signal) {
    switch (signal) {
        case Signal::terminate:
            return SIGTERM;
        case Signal::interrupt:
            return SIGINT;
        case Signal::hangup:
            return SIGHUP;
        case Signal::quit:
            return SIGQUIT;
        case Signal::kill:
            return SIGKILL;
    }
    return Error::unexpected(
        ErrorCode::invalidArgument, "Invalid signal value: {}",
        static_cast<u16>(signal)
    );
}

Result<Signal> fromUnixSignal(i32 unixSignal) {
    switch (unixSignal) {
        case SIGTERM:
            return Signal::terminate;
        case SIGINT:
            return Signal::interrupt;
        case SIGHUP:
            return Signal::hangup;
        case SIGQUIT:
            return Signal::quit;
        case SIGKILL:
            return Signal::kill;
    }
    return Error::unexpected(
        ErrorCode::invalidArgument, "Invalid Unix signal value: {}", unixSignal
    );
}

Opt<Error> UnixSignalManager::send(Signal signal, Pid pid) {
    if (auto unixSignal = toUnixSignal(signal); unixSignal) {
        ::kill(static_cast<pid_t>(pid), unixSignal.value());
        return Error::empty();
    } else {
        return unixSignal.error();
    }
}

Opt<Error> UnixSignalManager::registerHandler(Signal signal, Handler handler) {
    auto unixSignal = toUnixSignal(signal);
    if (not unixSignal) {
        return unixSignal.error();
    }

    log::info("registering handler for signal: {}", unixSignal.value());

    struct sigaction sa {};
    sa.sa_handler = [](i32 signum) {
        auto internalSignal = fromUnixSignal(signum);

        if (not internalSignal) {
            log::error(
                "Received invalid Unix signal with numeric value: {}", signum
            );
            return;
        }

        auto sig = static_cast<u16>(internalSignal.value());

        if (sig >= static_cast<u16>(Signal::count)) {
            log::error(
                "Received invalid signal with numeric value: {}/{}", sig, signum
            );
            return;
        }
        if (auto cb = s_handlers[sig]; cb) {
            cb();
        }
    };

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(unixSignal.value(), &sa, nullptr) < 0) {
        return Error{
            ErrorCode::osError, "Failed to register signal handler for {}",
            unixSignal.value()
        };
    }

    s_handlers[static_cast<u16>(signal)] = handler;
    return Error::empty();
}

void UnixSignalManager::removeHandler(Signal signal) {
    auto unixSignal = toUnixSignal(signal);
    if (not unixSignal) {
        log::error(
            "Failed to remove handler for signal: {}",
            unixSignal.error().message()
        );
        return;
    }

    struct sigaction sa {};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(unixSignal.value(), &sa, nullptr) < 0) {
        log::error(
            "Failed to remove signal handler for {}: {}", unixSignal.value(),
            strerror(errno)
        );
        return;
    }
    s_handlers[static_cast<u16>(signal)] = nullptr;
}

}  // namespace jstine
