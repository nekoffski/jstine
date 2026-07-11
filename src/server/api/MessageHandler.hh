#pragma once

#include "Message.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "storage/Database.hh"

namespace jstine {

class MessageHandler : public NonCopyable, public NonMovable {
    class Dispatcher : public NonCopyable, public NonMovable {
       public:
        explicit Dispatcher(Database& database);

        Response operator()(const PingRequestBody& body);
        Response operator()(const SetRequestBody& body);
        Response operator()(const GetRequestBody& body);
        Response operator()(const DelRequestBody& body);
        Response operator()(const ExistsRequestBody& body);

       private:
        Database& m_database;
    };

   public:
    explicit MessageHandler(Database& database);

    Response onRequest(const Request& request);

   private:
    Dispatcher m_dispatcher;
};

}  // namespace jstine
