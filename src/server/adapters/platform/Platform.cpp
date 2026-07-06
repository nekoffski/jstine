#include "Platform.hh"

#include "adapters/platform/unix/Signal.hh"
#include "core/OS.hh"

namespace jstine {

std::unique_ptr<SignalManager> createSignalManager() {
    if constexpr (constexpr auto os = detectOs();
                  os == OS::linux || os == OS::darwin) {
        return std::make_unique<UnixSignalManager>();
    } else {
        log::panic("Unsupported platform: {}", toString(os));
    }
}

}  // namespace jstine
