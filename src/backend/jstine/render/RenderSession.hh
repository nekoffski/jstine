#pragma once

#include <atomic>

#include "Integrator.hh"
#include "jstine/core/Concepts.hh"
#include "jstine/core/Core.hh"
#include "jstine/imaging/Film.hh"
#include "jstine/runtime/Thread.hh"

namespace jstine {

namespace details {

class IntegratorThread : public Thread {
   public:
    explicit IntegratorThread(const Str& ident, Integrator& integrator);

   private:
    Opt<Error> run() override;

    Integrator& m_integrator;
};

}  // namespace details

class RenderOrchestrator;

class RenderSession : public NonCopyable {
    friend class RenderOrchestrator;

   public:
    ~RenderSession();
    RenderSession(RenderSession&& oth) = default;

    bool finished() const;
    Opt<Error> wait();

    Result<FilmSnapshot> snapshot();

   private:
    explicit RenderSession(std::unique_ptr<Integrator> integrator);

    std::unique_ptr<Integrator> m_integrator;
    std::unique_ptr<details::IntegratorThread> m_runner;
};

}  // namespace jstine
