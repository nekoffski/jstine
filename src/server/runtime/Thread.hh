#pragma once

#include <thread>
#include <vector>

#include "core/Concepts.hh"
#include "core/Core.hh"

namespace jstine {

class Thread : public NonCopyable, public NonMovable {
   public:
    virtual ~Thread();

    void start();
    void join();

    virtual void cancel() {}

   protected:
    void sleepFor(std::chrono::milliseconds duration);

   private:
    virtual void run() = 0;

    std::thread m_thread;
};

class ThreadGroup : public NonCopyable, public NonMovable {
   public:
    void start();
    void join();
    void cancel();

    template <typename T, typename... Args>
        requires std::derived_from<T, Thread> &&
                 std::constructible_from<T, Args...>
    void add(Args&&... args) {
        m_threads.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

   private:
    std::vector<std::unique_ptr<Thread>> m_threads;
};

}  // namespace jstine
