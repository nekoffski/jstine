#pragma once

#include <memory>

#include "runtime/Signal.hh"

namespace jstine {

std::unique_ptr<SignalManager> createSignalManager();

}
