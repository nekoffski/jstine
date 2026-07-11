#include "Reaper.hh"

#include "core/Scope.hh"
#include "core/Time.hh"

namespace jstine {

Reaper::Reaper(const Config& config, Keyspace& keyspace)
    : Thread("StorageReaper"), m_config(config), m_keyspace(keyspace) {}

void Reaper::cancel() {
    m_running = false;
    m_cv.notify_all();
}

void Reaper::run() {
    const auto interval = m_config.storage().reaperInterval;
    log::info(
        "Starting reaper watchdog, reap interval: {} seconds", interval.count()
    );

    std::unique_lock lk{m_mutex};

    while (m_running) {
        if (m_cv.wait_for(lk, interval, [this] { return not m_running; })) {
            break;
        }

        lk.unlock();
        ON_SCOPE_EXIT { lk.lock(); };

        log::debug("Reaper pass start");
        reap();
        log::debug("Reaper pass end");
    }
}

void Reaper::reap() {}

}  // namespace jstine
