#pragma once

#include <thread>
#include <vector>

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class Thread : public NonCopyable, public NonMovable {
   public:
    explicit Thread(const Str& ident);
    virtual ~Thread();

    void start();
    void join();

    virtual void cancel() {}
    [[nodiscard]] virtual Opt<Error> init();

   protected:
    void sleepFor(std::chrono::milliseconds duration);

   private:
    virtual void run() = 0;

    Str m_ident;
    std::thread m_thread;
};

namespace {

template <typename Callback>
    requires Callable<Callback, void()>
class ThreadWrapper : public Thread {
   public:
    explicit ThreadWrapper(const Str& ident, Callback&& callback)
        : Thread(ident), m_callback(std::forward<Callback>(callback)) {}

   private:
    void run() override { m_callback(); }

    Callback m_callback;
};

}  // namespace

class ThreadGroup : public NonCopyable, public NonMovable {
   public:
    [[nodiscard]] Opt<Error> start();
    void join();
    void cancel();

    template <typename T, typename... Args>
        requires std::derived_from<T, Thread> &&
                 std::constructible_from<T, Args...>
    void add(Args&&... args) {
        m_threads.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    template <typename Callback>
        requires Callable<Callback, void()>
    void add(const Str& ident, Callback&& callback) {
        m_threads.push_back(
            std::make_unique<ThreadWrapper<Callback>>(
                ident, std::forward<Callback>(callback)
            )
        );
    }

   private:
    std::vector<std::unique_ptr<Thread>> m_threads;
};

}  // namespace jstine
