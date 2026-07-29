#include "StorageProxy.hh"

#include <future>
#include <utility>

namespace jstine {

namespace {

template <typename CommandType>
void failBecauseStorageIsUnavailable(CommandType& command) {
    command.callback(
        Error::unexpected(
            ErrorCode::storageUnavailable, "Storage executor is unavailable"
        )
    );
}

template <typename ResultType, typename CommandType>
ResultType submit(StorageCommandQueue& commandQueue, CommandType command) {
    std::promise<ResultType> promise;
    auto resultFuture = promise.get_future();

    auto callback = [promise = std::move(promise)](ResultType value) mutable {
        promise.set_value(std::move(value));
    };
    command.callback = std::move(callback);

    Command queuedCommand{std::move(command)};
    if (not commandQueue.push(std::move(queuedCommand))) {
        failBecauseStorageIsUnavailable(std::get<CommandType>(queuedCommand));
    }

    return resultFuture.get();
}

}  // namespace

StorageProxy::StorageProxy(StorageCommandQueue& commandQueue)
    : m_commandQueue(commandQueue) {}

Result<bool> StorageProxy::exists(CBytesView keyBytes) const {
    ExistsCommand command{};
    command.keyBytes = keyBytes;

    return submit<Result<bool>>(m_commandQueue, std::move(command));
}

Result<void> StorageProxy::remove(const std::vector<CBytesView>& keyBytesList) {
    RemoveCommand command{};
    command.keyBytesList = keyBytesList;

    return submit<Result<void>>(m_commandQueue, std::move(command));
}

Result<void> StorageProxy::set(CBytesView keyBytes, CBytesView valueBytes) {
    SetCommand command{};
    command.keyBytes = keyBytes;
    command.valueBytes = valueBytes;

    return submit<Result<void>>(m_commandQueue, std::move(command));
}

Result<Bytes> StorageProxy::get(CBytesView keyBytes) const {
    GetCommand command{};
    command.keyBytes = keyBytes;

    return submit<Result<Bytes>>(m_commandQueue, std::move(command));
}

}  // namespace jstine
