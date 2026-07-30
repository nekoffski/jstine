#pragma once

#include <exception>
#include <optional>
#include <utility>

#include "Asio.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "storage/api/StorageCommandQueue.hh"
#include "storage/api/StorageTransaction.hh"

namespace jstine {

class AsioStorageProxy : public NonCopyable, public NonMovable {
   public:
    explicit AsioStorageProxy(StorageCommandQueue& commandQueue);

    asio::awaitable<Result<bool>> exists(CBytesView keyBytes) const;
    asio::awaitable<Result<void>> remove(CBytesView keyBytes);

    asio::awaitable<Result<void>> set(
        CBytesView keyBytes, CBytesView valueBytes
    );

    asio::awaitable<Result<Bytes>> get(CBytesView keyBytes) const;

    template <detail::AtomicCallable Callable>
    asio::awaitable<detail::AtomicResult<Callable>> atomically(
        Callable&& callable
    ) {
        return submitAtomically(
            std::decay_t<Callable>(std::forward<Callable>(callable))
        );
    }

   private:
    template <typename ResultType>
    using AtomicCompletion = std::optional<ResultType>;

    template <detail::AtomicCallable Callable>
    static detail::AtomicResult<Callable> invokeTransaction(
        Callable& callable, Result<std::reference_wrapper<Database>> database
    ) {
        if (not database) {
            return Error::unexpected(database.error());
        }

        StorageTransaction transaction{database->get()};
        return detail::invokeAtomically(callable, transaction);
    }

    template <typename Executor, typename Handler, typename ResultType>
    static void postResult(
        Executor executor, Handler handler, ResultType result
    ) {
        asio::post(
            executor, [handler = std::move(handler),
                       result = std::move(result)]() mutable {
                handler(
                    std::exception_ptr{},
                    AtomicCompletion<ResultType>{std::move(result)}
                );
            }
        );
    }

    template <typename Executor, typename Handler>
    static void postException(
        Executor executor, Handler handler, std::exception_ptr exception
    ) {
        asio::post(
            executor, [handler = std::move(handler), exception]() mutable {
                handler(exception, std::nullopt);
            }
        );
    }

    template <detail::AtomicCallable Callable, typename Handler>
    static TransactionCommand makeTransactionCommand(
        Callable callable, Handler handler
    ) {
        using ResultType = detail::AtomicResult<Callable>;

        auto executor = asio::get_associated_executor(handler);
        TransactionCommand command{};
        command.callback = [callable = std::move(callable), executor,
                            handler = std::move(handler)](
                               Result<std::reference_wrapper<Database>> database
                           ) mutable {
            try {
                ResultType result =
                    invokeTransaction(callable, std::move(database));
                postResult(executor, std::move(handler), std::move(result));
            } catch (...) {
                postException(
                    executor, std::move(handler), std::current_exception()
                );
            }
        };
        return command;
    }

    template <detail::AtomicCallable Callable>
    asio::awaitable<detail::AtomicResult<Callable>> submitAtomically(
        Callable callable
    ) {
        using ResultType = detail::AtomicResult<Callable>;
        using CompletionValue = AtomicCompletion<ResultType>;

        auto token = asio::use_awaitable;
        auto result = co_await asio::async_initiate<
            asio::use_awaitable_t<>, void(std::exception_ptr, CompletionValue)>(
            [this, callable = std::move(callable)](auto handler) mutable {
                schedule(makeTransactionCommand(
                    std::move(callable), std::move(handler)
                ));
            },
            token
        );

        co_return std::move(*result);
    }

    void schedule(TransactionCommand&& command);

    StorageCommandQueue& m_commandQueue;
};

}  // namespace jstine
