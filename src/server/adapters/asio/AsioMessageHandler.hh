#pragma once

#include "Asio.hh"
#include "AsioStorageProxy.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "proto/Message.hh"
#include "storage/api/StorageCommandQueue.hh"

namespace jstine {

class AsioMessageHandler : public NonCopyable, public NonMovable {
    class Dispatcher : public NonCopyable, public NonMovable {
       public:
        explicit Dispatcher(StorageCommandQueue& commandQueue);

        asio::awaitable<Response> operator()(const PingRequestBody& body);
        asio::awaitable<Response> operator()(const SetRequestBody& body);
        asio::awaitable<Response> operator()(const GetRequestBody& body);
        asio::awaitable<Response> operator()(const DelRequestBody& body);
        asio::awaitable<Response> operator()(const ExistsRequestBody& body);
        asio::awaitable<Response> operator()(const TtlRequestBody& body);
        asio::awaitable<Response> operator()(const PersistRequestBody& body);
        asio::awaitable<Response> operator()(const ExpireRequestBody& body);

       private:
        AsioStorageProxy m_storageProxy;
    };

   public:
    explicit AsioMessageHandler(StorageCommandQueue& commandQueue);

    asio::awaitable<Response> onRequest(const Request& request);

   private:
    Dispatcher m_dispatcher;
};

}  // namespace jstine
