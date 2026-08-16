#pragma once

#include <thread>
#include <vector>

#include "jstine/core/Concepts.hh"
#include "jstine/core/Core.hh"
#include "jstine/core/Error.hh"

namespace jstine {

class Thread : public NonCopyable, public NonMovable {
   public:
    enum class Status { running, failed, finished };

    explicit Thread(const Str& ident);
    virtual ~Thread();

    Opt<Error> join();
    Status status() const;
    virtual void cancel() {}

   protected:
    void sleepFor(std::chrono::milliseconds duration);

   private:
    virtual Opt<Error> run() = 0;
    void go();

    Str m_ident;
    Status m_status{Status::running};
    Opt<Error> m_error;
    std::thread m_thread;
};

}  // namespace jstine
