#pragma once

#include <optional>
#include <variant>

#include "core/Error.hh"
#include "core/Functional.hh"
#include "storage/Database.hh"
#include "storage/kv/Key.hh"
#include "storage/kv/Value.hh"

namespace jstine {

template <typename ReturnType>
struct CommandBase {
    using Callback = MoveOnlyFunction<void(Result<ReturnType>)>;

    Callback callback;
};

struct GetCommand : CommandBase<Bytes> {
    CBytesView keyBytes;
};

struct SetCommand : CommandBase<void> {
    CBytesView keyBytes;
    CBytesView valueBytes;
};

struct RemoveCommand : CommandBase<void> {
    CBytesView keyBytes;
};

struct ExistsCommand : CommandBase<bool> {
    CBytesView keyBytes;
};

struct TransactionCommand : CommandBase<std::reference_wrapper<Database>> {};

using Command = std::variant<
    GetCommand, SetCommand, RemoveCommand, ExistsCommand, TransactionCommand>;

}  // namespace jstine
