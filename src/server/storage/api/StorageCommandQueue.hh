#pragma once

#include "Commands.hh"
#include "runtime/ThreadSafeQueue.hh"

namespace jstine {

class StorageCommandQueue {
   public:
    explicit StorageCommandQueue(ThreadSafeQueue<Command>& commandQueue);

    [[nodiscard]] bool push(Command&& command);

   private:
    ThreadSafeQueue<Command>& m_commandQueue;
};

}  // namespace jstine
