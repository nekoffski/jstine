#pragma once

#include <span>

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "storage/api/StorageCommandQueue.hh"

namespace jstine {

class StorageProxy : public NonCopyable, public NonMovable {
   public:
    explicit StorageProxy(StorageCommandQueue& commandQueue);

    Result<bool> exists(std::span<const Byte> keyBytes) const;
    Result<void> remove(const std::vector<std::span<const Byte>>& keyBytesList);

    Result<void> set(
        std::span<const Byte> keyBytes, std::span<const Byte> valueBytes
    );

    Result<Bytes> get(std::span<const Byte> keyBytes) const;

   private:
    StorageCommandQueue& m_commandQueue;
};

}  // namespace jstine
