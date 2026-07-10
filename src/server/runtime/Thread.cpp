#include "Thread.hh"

#include <functional>

#include "core/Error.hh"
#include "core/Log.hh"
#include "core/Profiler.hh"

namespace jstine {

void Thread::start() {
    m_thread = std::thread([this] {
        JSTINE_PROFILE_REGISTER_THREAD();

        try {
            run();
        } catch (const std::exception& e) {
            log::error("Standard exception in thread: {}", e.what());
        } catch (...) {
            log::error("Unknown exception in thread");
        }
    });
}

void Thread::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

Opt<Error> Thread::init() { return Error::empty(); }

void Thread::sleepFor(std::chrono::milliseconds duration) {
    std::this_thread::sleep_for(duration);
}

Thread::~Thread() { join(); }

Opt<Error> ThreadGroup::start() {
    for (auto& thread : m_threads) {
        if (auto err = thread->init(); err) {
            return err;
        }
    }
    for (auto& thread : m_threads) {
        thread->start();
    }
    return Error::empty();
}

void ThreadGroup::join() {
    for (auto& thread : m_threads) {
        thread->join();
    }
}

void ThreadGroup::cancel() {
    for (auto& thread : m_threads) {
        thread->cancel();
    }
}

}  // namespace jstine
