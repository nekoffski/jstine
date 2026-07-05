#pragma once

#include "Message.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "storage/StorageManager.hh"

namespace jstine {

class MessageHandler : public NonCopyable, public NonMovable {
    class Dispatcher {
       public:
        explicit Dispatcher(StorageManager& storageManager);

        Response operator()(const PingRequestBody& body);
        Response operator()(const SetRequestBody& body);
        Response operator()(const GetRequestBody& body);
        Response operator()(const DelRequestBody& body);
        Response operator()(const ExistsRequestBody& body);

       private:
        StorageManager& m_storageManager;
    };

   public:
    explicit MessageHandler(StorageManager& storageManager);

    Response onRequest(const Request& request);

   private:
    Dispatcher m_dispatcher;
};

}  // namespace jstine
