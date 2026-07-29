#pragma once

#include <optional>
#include <variant>

#include "core/Error.hh"
#include "core/Functional.hh"
#include "storage/kv/Key.hh"
#include "storage/kv/Value.hh"

namespace jstine {

template <typename ReturnType>
struct CommandBase {
    using Callback = MoveOnlyFunction<void(Result<ReturnType>)>;

    Callback callback;
};

struct GetCommand : CommandBase<Bytes> {
    std::span<const Byte> keyBytes;
};

struct SetCommand : CommandBase<void> {
    std::span<const Byte> keyBytes;
    std::span<const Byte> valueBytes;
};

struct RemoveCommand : CommandBase<void> {
    std::vector<std::span<const Byte>> keyBytesList;
};

struct ExistsCommand : CommandBase<bool> {
    std::span<const Byte> keyBytes;
};

using Command =
    std::variant<GetCommand, SetCommand, RemoveCommand, ExistsCommand>;

}  // namespace jstine
