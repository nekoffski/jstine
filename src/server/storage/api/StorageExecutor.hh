#pragma once

#include "runtime/Thread.hh"
#include "runtime/ThreadSafeQueue.hh"
#include "storage/Database.hh"
#include "storage/api/Commands.hh"

namespace jstine {

class StorageExecutor : public Thread {
   public:
    StorageExecutor(Database& database, ThreadSafeQueue<Command>& commandQueue);

    void cancel() override;

   private:
    void run() override;

    void execute(GetCommand&& command);
    void execute(SetCommand&& command);
    void execute(RemoveCommand&& command);
    void execute(ExistsCommand&& command);
    void execute(TransactionCommand&& command);

    Database& m_database;
    ThreadSafeQueue<Command>& m_commandQueue;
};

}  // namespace jstine
