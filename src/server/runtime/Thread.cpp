#include "Thread.hh"

#include "core/Error.hh"
#include "core/Log.hh"

namespace jstine {

void Thread::start() {
    m_thread = std::thread([this] {
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

void Thread::sleepFor(std::chrono::milliseconds duration) {
    std::this_thread::sleep_for(duration);
}

Thread::~Thread() { join(); }

void ThreadGroup::start() {
    for (auto& thread : m_threads) {
        thread->start();
    }
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
