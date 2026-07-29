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

    Result<bool> exists(CBytesView keyBytes) const;
    Result<void> remove(const std::vector<CBytesView>& keyBytesList);

    Result<void> set(CBytesView keyBytes, CBytesView valueBytes);

    Result<Bytes> get(CBytesView keyBytes) const;

   private:
    StorageCommandQueue& m_commandQueue;
};

}  // namespace jstine
