#include "MessageHandler.hh"

#include "core/Functional.hh"

namespace jstine {

MessageHandler::MessageHandler(StorageManager& storageManager)
    : m_dispatcher(storageManager) {}

Response MessageHandler::onRequest(const Request& request) {
    return std::visit(m_dispatcher, request.body);
}

MessageHandler::Dispatcher::Dispatcher(StorageManager& storageManager)
    : m_storageManager(storageManager) {}

Response MessageHandler::Dispatcher::operator()(const PingRequestBody& body) {
    return Response::ok(body.payload);
}

Response MessageHandler::Dispatcher::operator()(const SetRequestBody& body) {
    if (auto err = m_storageManager.set(body.key, body.value); err) {
        return Response::error(*err);
    }
    return Response::ok();
}

Response MessageHandler::Dispatcher::operator()(const GetRequestBody& body) {
    return Response::error(
        ErrorCode::notImplementedYet, "Get request not implemented yet"
    );
}

Response MessageHandler::Dispatcher::operator()(const DelRequestBody& body) {
    if (not m_storageManager.exists(body.key)) {
        return Response::error(ErrorCode::notFound, "Key does not exist");
    }

    m_storageManager.remove(body.key);
    return Response::ok();
}

Response MessageHandler::Dispatcher::operator()(const ExistsRequestBody& body) {
    if (m_storageManager.exists(body.key)) {
        return Response::ok();
    }
    return Response::error(ErrorCode::notFound, "Key does not exist");
}

}  // namespace jstine
