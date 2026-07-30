#pragma once

#include <functional>
#include <type_traits>
#include <utility>

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "storage/Database.hh"

namespace jstine {

class StorageProxy;
class AsioStorageProxy;

// A synchronous view of storage that is valid only for the duration of an
// atomically() callback. Callbacks run on the storage executor and must not
// block, suspend, re-enter a storage proxy, or retain this object.
class StorageTransaction : public NonCopyable, public NonMovable {
   public:
    bool exists(CBytesView keyBytes) const;
    void remove(CBytesView keyBytes);
    Result<void> set(CBytesView keyBytes, CBytesView valueBytes);
    Result<Bytes> get(CBytesView keyBytes) const;

   private:
    friend StorageProxy;
    friend AsioStorageProxy;

    explicit StorageTransaction(Database& database);

    Database& m_database;
};

namespace detail {

template <typename T>
struct IsResult : std::false_type {};

template <typename T>
struct IsResult<Result<T>> : std::true_type {};

template <typename T>
inline constexpr bool isResult = IsResult<std::remove_cvref_t<T>>::value;

template <typename InvocationResult>
struct NormalizeAtomicResult {
    using Type = Result<std::remove_cvref_t<InvocationResult>>;
};

template <>
struct NormalizeAtomicResult<void> {
    using Type = Result<void>;
};

template <typename T>
struct NormalizeAtomicResult<Result<T>> {
    using Type = Result<T>;
};

template <typename Callable>
using AtomicInvocationResult =
    std::invoke_result_t<std::decay_t<Callable>&, StorageTransaction&>;

template <typename Callable>
using AtomicResult = typename NormalizeAtomicResult<
    std::remove_cvref_t<AtomicInvocationResult<Callable>>>::Type;

template <typename Callable>
concept AtomicCallable =
    std::invocable<std::decay_t<Callable>&, StorageTransaction&>;

template <AtomicCallable Callable>
AtomicResult<Callable> invokeAtomically(
    Callable& callable, StorageTransaction& transaction
) {
    using InvocationResult = AtomicInvocationResult<Callable>;

    if constexpr (std::is_void_v<InvocationResult>) {
        std::invoke(callable, transaction);
        return {};
    } else if constexpr (isResult<InvocationResult>) {
        return std::invoke(callable, transaction);
    } else {
        return std::invoke(callable, transaction);
    }
}

}  // namespace detail

}  // namespace jstine
