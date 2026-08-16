#pragma once

#include <atomic>

#include "Integrator.hh"
#include "RenderContext.hh"
#include "RenderRequest.hh"
#include "jstine/core/Concepts.hh"
#include "jstine/core/Core.hh"
#include "jstine/imaging/Film.hh"
#include "jstine/runtime/Thread.hh"

namespace jstine {

namespace details {

class IntegratorThread : public Thread {
   public:
    explicit IntegratorThread(
        const Str& ident, Integrator& integrator, Film& film
    );

   private:
    Result<void> run() override;

    Integrator& m_integrator;
    Film& m_film;
};

}  // namespace details

class RenderOrchestrator;

class RenderSession : public NonCopyable {
    friend class RenderOrchestrator;

   public:
    ~RenderSession();
    RenderSession(RenderSession&& oth) = default;

    bool finished() const;
    Result<void> wait();

    FilmSnapshot snapshot();

   private:
    explicit RenderSession(
        std::unique_ptr<Integrator> integrator, const RenderRequest& req
    );

    std::unique_ptr<Integrator> m_integrator;
    std::unique_ptr<Film> m_film;
    std::unique_ptr<details::IntegratorThread> m_runner;
};

}  // namespace jstine
