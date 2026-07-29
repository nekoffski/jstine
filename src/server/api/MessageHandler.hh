#pragma once

#include "Message.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "storage/Storage.hh"

namespace jstine {

class MessageHandler : public NonCopyable, public NonMovable {
    class Dispatcher : public NonCopyable, public NonMovable {
       public:
        explicit Dispatcher(Storage& storage);

        Response operator()(const PingRequestBody& body);
        Response operator()(const SetRequestBody& body);
        Response operator()(const GetRequestBody& body);
        Response operator()(const DelRequestBody& body);
        Response operator()(const ExistsRequestBody& body);

       private:
        Storage& m_storage;
    };

   public:
    explicit MessageHandler(Storage& storage);

    Response onRequest(const Request& request);

   private:
    Dispatcher m_dispatcher;
};

}  // namespace jstine
