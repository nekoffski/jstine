#include "RenderSession.hh"

namespace jstine {

RenderSession::RenderSession(
    std::unique_ptr<Integrator> integrator, const RenderRequest& req
)
    : m_integrator(std::move(integrator)),
      m_film(std::make_unique<Film>(req.film)),
      m_runner(
          std::make_unique<details::IntegratorThread>(
              "xyz", *m_integrator, *m_film
          )
      ) {}

RenderSession::~RenderSession() {
    if (m_runner && not finished()) {
        m_runner->cancel();
        m_runner->join();
    }
}

bool RenderSession::finished() const {
    return m_runner->status() == Thread::Status::finished ||
           m_runner->status() == Thread::Status::failed;
}

Result<void> RenderSession::wait() { return m_runner->join(); }

FilmSnapshot RenderSession::snapshot() { return m_film->snapshot(); }

details::IntegratorThread::IntegratorThread(
    const Str& ident, Integrator& integrator, Film& film
)
    : Thread(fmt::format("Session_{}", ident)),
      m_integrator(integrator),
      m_film(film) {}

Result<void> details::IntegratorThread::run() {
    RenderContext ctx{m_film};
    return m_integrator.render(ctx);
}

}  // namespace jstine
