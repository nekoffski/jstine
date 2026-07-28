#pragma once

#include <variant>

#include "EmptyValue.hh"
#include "StrValue.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "core/Time.hh"

namespace jstine {

enum class ValueKind { empty = 0, str = 1, list = 2 };

struct Metadata {
    ValueKind kind;
    Clock::time_point accessedAt;
    Clock::time_point modifiedAt;
    u64 size;
};

using Value = std::variant<EmptyValue, StrValue>;

}  // namespace jstine
