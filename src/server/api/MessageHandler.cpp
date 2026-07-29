#include "MessageHandler.hh"

#include "core/Functional.hh"
#include "core/Profiler.hh"

namespace jstine {

MessageHandler::MessageHandler(Storage& storage) : m_dispatcher(storage) {}

Response MessageHandler::onRequest(const Request& request) {
    JSTINE_PROFILE_FUNCTION();
    return std::visit(m_dispatcher, request.body);
}

MessageHandler::Dispatcher::Dispatcher(Storage& storage)
    : m_storage(storage) {}

Response MessageHandler::Dispatcher::operator()(const PingRequestBody& body) {
    JSTINE_PROFILE_REGION("PingRequest");
    return Response::ok(body.payload);
}

Response MessageHandler::Dispatcher::operator()(const SetRequestBody& body) {
    JSTINE_PROFILE_REGION("SetRequest");
    if (auto err = m_storage.set(body.key, body.value); err) {
        return Response::error(*err);
    }
    return Response::ok();
}

Response MessageHandler::Dispatcher::operator()(const GetRequestBody& body) {
    JSTINE_PROFILE_REGION("GetRequest");
    if (auto value = m_storage.get(body.key); value) {
        return Response::ok(*value);
    } else {
        return Response::error(value.error());
    }
}

Response MessageHandler::Dispatcher::operator()(const DelRequestBody& body) {
    JSTINE_PROFILE_REGION("DelRequest");
    if (not m_storage.exists(body.key)) {
        return Response::error(ErrorCode::notFound, "Key does not exist");
    }

    m_storage.remove(body.key);
    return Response::ok();
}

Response MessageHandler::Dispatcher::operator()(const ExistsRequestBody& body) {
    JSTINE_PROFILE_REGION("ExistsRequest");
    if (m_storage.exists(body.key)) {
        return Response::ok();
    }
    return Response::error(ErrorCode::notFound, "Key does not exist");
}

}  // namespace jstine
