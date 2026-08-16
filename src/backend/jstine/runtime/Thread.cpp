#include "Thread.hh"

#include <functional>

#include "jstine/core/Log.hh"
#include "jstine/core/Profiler.hh"
#include "jstine/core/Scope.hh"

namespace jstine {

Result<void> Thread::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
    if (m_error) {
        return Error::unexpected(m_error.value());
    }
    return {};
}

Thread::Status Thread::status() const { return m_status.load(); }

void Thread::sleepFor(std::chrono::milliseconds duration) {
    std::this_thread::sleep_for(duration);
}

Thread::Thread(const Str& ident) : m_ident(ident) {}

Thread::~Thread() { join(); }

void Thread::start() {
    m_thread = std::thread([&] { go(); });
    m_status = Status::running;
}

void Thread::go() {
    JSTINE_PROFILE_REGISTER_THREAD();
    log::info("{} - thread starting", m_ident);

    try {
        if (auto res = run(); not res) {
            m_status = Status::failed;
            m_error = res.error();
        }
    } catch (const std::exception& e) {
        log::error("{} - standard exception in thread: {}", m_ident, e.what());
        m_status = Status::failed;
        m_error.emplace(
            ErrorCode::threadWorkerFailed, "Worker failed: {}", e.what()
        );
        return;
    } catch (...) {
        log::error("{} - unknown exception in thread", m_ident);
        m_status = Status::failed;
        m_error.emplace(
            ErrorCode::threadWorkerFailed, "Unknown exception in thread"
        );
        return;
    }

    m_status = m_error ? Status::failed : Status::finished;
    log::info("{} - thread exiting", m_ident);
}

}  // namespace jstine
