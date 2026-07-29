#pragma once

#include "Asio.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "storage/api/StorageCommandQueue.hh"

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

   private:
    StorageCommandQueue& m_commandQueue;
};

}  // namespace jstine
