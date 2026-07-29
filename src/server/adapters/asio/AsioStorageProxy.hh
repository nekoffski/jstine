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

    asio::awaitable<Result<bool>> exists(std::span<const Byte> keyBytes) const;
    asio::awaitable<Result<void>> remove(std::span<const Byte> keyBytes);

    asio::awaitable<Result<void>> set(
        std::span<const Byte> keyBytes, std::span<const Byte> valueBytes
    );

    asio::awaitable<Result<Bytes>> get(std::span<const Byte> keyBytes) const;

   private:
    StorageCommandQueue& m_commandQueue;
};

}  // namespace jstine
