#pragma once

#include <exception>
#include <future>
#include <span>
#include <utility>

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "storage/api/StorageCommandQueue.hh"
#include "storage/api/StorageTransaction.hh"

namespace jstine {

class StorageProxy : public NonCopyable, public NonMovable {
   public:
    explicit StorageProxy(StorageCommandQueue& commandQueue);

    Result<bool> exists(CBytesView keyBytes) const;
    Result<void> remove(CBytesView keyBytes);

    Result<void> set(CBytesView keyBytes, CBytesView valueBytes);

    Result<Bytes> get(CBytesView keyBytes) const;

    template <detail::AtomicCallable Callable>
    detail::AtomicResult<Callable> atomically(Callable&& callable) {
        using ResultType = detail::AtomicResult<Callable>;

        std::promise<ResultType> promise;
        auto resultFuture = promise.get_future();

        TransactionCommand command{};
        command.callback =
            [callable =
                 std::decay_t<Callable>(std::forward<Callable>(callable)),
             promise = std::move(promise)](
                Result<std::reference_wrapper<Database>> database
            ) mutable {
                if (not database) {
                    promise.set_value(Error::unexpected(database.error()));
                    return;
                }

                try {
                    StorageTransaction transaction{database->get()};
                    promise.set_value(
                        detail::invokeAtomically(callable, transaction)
                    );
                } catch (...) {
                    promise.set_exception(std::current_exception());
                }
            };

        schedule(std::move(command));
        return resultFuture.get();
    }

   private:
    void schedule(TransactionCommand&& command);

    StorageCommandQueue& m_commandQueue;
};

}  // namespace jstine
