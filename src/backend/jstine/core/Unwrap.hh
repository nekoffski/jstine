#pragma once

#include "Log.hh"

namespace jstine {

template <typename T>
T unwrap(Result<T>&& result) {
    log::expect(result);
    return std::move(*result);
}

}  // namespace jstine
