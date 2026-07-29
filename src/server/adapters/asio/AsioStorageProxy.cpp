#include "AsioStorageProxy.hh"

namespace jstine {

namespace {

template <typename ResultType, typename CommandType>
asio::awaitable<ResultType> submit(
    StorageCommandQueue& commandQueue, CommandType command
) {
    asio::use_awaitable_t<> token;
    co_return co_await asio::async_initiate<
        asio::use_awaitable_t<>, void(ResultType)>(
        [&commandQueue, command = std::move(command)](auto handler) mutable {
            auto executor = asio::get_associated_executor(handler);
            command.callback = [executor, handler = std::move(handler)](
                                   ResultType result
                               ) mutable {
                asio::post(
                    executor, [handler = std::move(handler),
                               result = std::move(result)]() mutable {
                        handler(std::move(result));
                    }
                );
            };

            Command queuedCommand{std::move(command)};

            if (not commandQueue.push(std::move(queuedCommand))) {
                std::get<CommandType>(queuedCommand)
                    .callback(
                        Error::unexpected(
                            ErrorCode::storageUnavailable,
                            "Storage executor is unavailable"
                        )
                    );
            }
        },
        token
    );
}

}  // namespace

AsioStorageProxy::AsioStorageProxy(StorageCommandQueue& commandQueue)
    : m_commandQueue(commandQueue) {}

asio::awaitable<Result<bool>> AsioStorageProxy::exists(
    std::span<const Byte> keyBytes
) const {
    ExistsCommand command{};
    command.keyBytes = keyBytes;

    co_return co_await submit<Result<bool>>(m_commandQueue, std::move(command));
}

asio::awaitable<Result<void>> AsioStorageProxy::remove(
    std::span<const Byte> keyBytes
) {
    RemoveCommand command{};
    command.keyBytes = keyBytes;

    co_return co_await submit<Result<void>>(m_commandQueue, std::move(command));
}

asio::awaitable<Result<void>> AsioStorageProxy::set(
    std::span<const Byte> keyBytes, std::span<const Byte> valueBytes
) {
    SetCommand command{};
    command.keyBytes = keyBytes;
    command.valueBytes = valueBytes;

    co_return co_await submit<Result<void>>(m_commandQueue, std::move(command));
}

asio::awaitable<Result<Bytes>> AsioStorageProxy::get(
    std::span<const Byte> keyBytes
) const {
    GetCommand command{};
    command.keyBytes = keyBytes;

    co_return co_await submit<Result<Bytes>>(
        m_commandQueue, std::move(command)
    );
}

}  // namespace jstine
