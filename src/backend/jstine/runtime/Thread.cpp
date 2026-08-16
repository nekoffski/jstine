#include "Thread.hh"

#include <functional>

#include "jstine/core/Log.hh"
#include "jstine/core/Profiler.hh"
#include "jstine/core/Scope.hh"

namespace jstine {

Opt<Error> Thread::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
    return m_error;
}

Thread::Status Thread::status() const { return m_status; }

void Thread::sleepFor(std::chrono::milliseconds duration) {
    std::this_thread::sleep_for(duration);
}

Thread::Thread(const Str& ident) : m_ident(ident), m_thread([this] { go(); }) {}

Thread::~Thread() { join(); }

void Thread::go() {
    JSTINE_PROFILE_REGISTER_THREAD();
    log::info("{} - thread starting", m_ident);

    try {
        ON_SCOPE_FAIL { m_status = Status::failed; };
        m_error = run();
    } catch (const std::exception& e) {
        log::error("{} - standard exception in thread: {}", m_ident, e.what());
        m_error.emplace(
            ErrorCode::threadWorkerFailed, "Worker failed: {}", e.what()
        );
        return;
    } catch (...) {
        log::error("{} - unknown exception in thread", m_ident);
        m_error.emplace(
            ErrorCode::threadWorkerFailed, "Unknown exception in thread"
        );
        return;
    }

    m_status = Status::finished;
    log::info("{} - thread exiting", m_ident);
}

}  // namespace jstine
