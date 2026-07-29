#include "StorageCommandQueue.hh"

namespace jstine {

StorageCommandQueue::StorageCommandQueue(ThreadSafeQueue<Command>& commandQueue)
    : m_commandQueue(commandQueue) {}

bool StorageCommandQueue::push(Command&& command) {
    return m_commandQueue.push(std::move(command));
}

}  // namespace jstine
