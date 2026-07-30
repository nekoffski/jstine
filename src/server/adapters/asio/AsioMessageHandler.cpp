#include "AsioMessageHandler.hh"

namespace jstine {

AsioMessageHandler::AsioMessageHandler(StorageCommandQueue& commandQueue)
    : m_dispatcher(commandQueue) {}

asio::awaitable<Response> AsioMessageHandler::onRequest(
    const Request& request
) {
    co_return co_await std::visit(m_dispatcher, request.body);
}

AsioMessageHandler::Dispatcher::Dispatcher(StorageCommandQueue& commandQueue)
    : m_storageProxy(commandQueue) {}

asio::awaitable<Response> AsioMessageHandler::Dispatcher::operator()(
    const PingRequestBody& body
) {
    co_return Response::ok(body.payload);
}

asio::awaitable<Response> AsioMessageHandler::Dispatcher::operator()(
    const SetRequestBody& body
) {
    if (auto result = co_await m_storageProxy.set(body.key, body.value);
        not result) {
        co_return Response::error(result.error());
    }
    co_return Response::ok();
}

asio::awaitable<Response> AsioMessageHandler::Dispatcher::operator()(
    const GetRequestBody& body
) {
    if (auto value = co_await m_storageProxy.get(body.key); value) {
        co_return Response::ok(*value);
    } else {
        co_return Response::error(value.error());
    }
}

asio::awaitable<Response> AsioMessageHandler::Dispatcher::operator()(
    const DelRequestBody& body
) {
    if (auto exists = co_await m_storageProxy.exists(body.key); not exists) {
        co_return Response::error(exists.error());
    } else if (not *exists) {
        co_return Response::error(ErrorCode::notFound, "Key does not exist");
    }

    if (auto result = co_await m_storageProxy.remove(body.key); not result) {
        co_return Response::error(result.error());
    }
    co_return Response::ok();
}

asio::awaitable<Response> AsioMessageHandler::Dispatcher::operator()(
    const ExistsRequestBody& body
) {
    if (auto exists = co_await m_storageProxy.exists(body.key); not exists) {
        co_return Response::error(exists.error());
    } else if (*exists) {
        co_return Response::ok();
    }
    co_return Response::error(ErrorCode::notFound, "Key does not exist");
}

asio::awaitable<Response> AsioMessageHandler::Dispatcher::operator()(
    const TtlRequestBody&
) {
    co_return Response::error(
        ErrorCode::notImplementedYet, "TTL is not implemented yet"
    );
}

asio::awaitable<Response> AsioMessageHandler::Dispatcher::operator()(
    const PersistRequestBody&
) {
    co_return Response::error(
        ErrorCode::notImplementedYet, "PERSIST is not implemented yet"
    );
}

asio::awaitable<Response> AsioMessageHandler::Dispatcher::operator()(
    const ExpireRequestBody&
) {
    co_return Response::error(
        ErrorCode::notImplementedYet, "EXPIRE is not implemented yet"
    );
}

}  // namespace jstine
