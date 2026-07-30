#include "StorageExecutor.hh"

namespace jstine {

StorageExecutor::StorageExecutor(
    Database& database, ThreadSafeQueue<Command>& commandQueue
)
    : Thread("StorageExecutor"),
      m_database(database),
      m_commandQueue(commandQueue) {}

void StorageExecutor::cancel() { m_commandQueue.close(); }

void StorageExecutor::run() {
    while (auto command = m_commandQueue.pop()) {
        std::visit(
            [this](auto&& item) { execute(std::move(item)); },
            std::move(*command)
        );
    }
}

void StorageExecutor::execute(GetCommand&& command) {
    if (auto value = m_database.get(command.keyBytes); value) {
        command.callback(Bytes(value->begin(), value->end()));
    } else {
        command.callback(Error::unexpected(value.error()));
    }
}

void StorageExecutor::execute(SetCommand&& command) {
    if (auto error = m_database.set(command.keyBytes, command.valueBytes);
        error) {
        command.callback(Error::unexpected(*error));
    } else {
        command.callback({});
    }
}

void StorageExecutor::execute(RemoveCommand&& command) {
    m_database.remove(command.keyBytes);
    command.callback({});
}

void StorageExecutor::execute(ExistsCommand&& command) {
    command.callback(m_database.exists(command.keyBytes));
}

void StorageExecutor::execute(TransactionCommand&& command) {
    command.callback(std::ref(m_database));
}

}  // namespace jstine
