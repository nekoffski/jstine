#include "RenderSession.hh"

namespace jstine {

RenderSession::RenderSession(std::unique_ptr<Integrator> integrator)
    : m_integrator(std::move(integrator)),
      m_runner(
          std::make_unique<details::IntegratorThread>("xyz", *m_integrator)
      ) {}

RenderSession::~RenderSession() {
    if (not finished()) {
        m_runner->cancel();
        m_runner->join();
    }
}

bool RenderSession::finished() const {
    return m_runner->status() == Thread::Status::finished ||
           m_runner->status() == Thread::Status::failed;
}

Opt<Error> RenderSession::wait() { return m_runner->join(); }

Result<FilmSnapshot> RenderSession::snapshot() {}

details::IntegratorThread::IntegratorThread(
    const Str& ident, Integrator& integrator
)
    : Thread(fmt::format("Session_{}", ident)), m_integrator(integrator) {}

Opt<Error> details::IntegratorThread::run() { return {}; }

}  // namespace jstine
