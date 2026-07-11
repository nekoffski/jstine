#include "MessageHandler.hh"

#include "core/Functional.hh"
#include "core/Profiler.hh"

namespace jstine {

MessageHandler::MessageHandler(Database& database) : m_dispatcher(database) {}

Response MessageHandler::onRequest(const Request& request) {
    JSTINE_PROFILE_FUNCTION();
    return std::visit(m_dispatcher, request.body);
}

MessageHandler::Dispatcher::Dispatcher(Database& database)
    : m_database(database) {}

Response MessageHandler::Dispatcher::operator()(const PingRequestBody& body) {
    JSTINE_PROFILE_REGION("PingRequest");
    return Response::ok(body.payload);
}

Response MessageHandler::Dispatcher::operator()(const SetRequestBody& body) {
    JSTINE_PROFILE_REGION("SetRequest");
    if (auto err = m_database.set(Key{body.key}, body.value); err) {
        return Response::error(*err);
    }
    return Response::ok();
}

Response MessageHandler::Dispatcher::operator()(const GetRequestBody& body) {
    JSTINE_PROFILE_REGION("GetRequest");
    if (auto value = m_database.get(Key{body.key}); value) {
        return Response::ok(*value);
    } else {
        return Response::error(value.error());
    }
}

Response MessageHandler::Dispatcher::operator()(const DelRequestBody& body) {
    JSTINE_PROFILE_REGION("DelRequest");
    Key key{body.key};
    if (not m_database.exists(key)) {
        return Response::error(ErrorCode::notFound, "Key does not exist");
    }

    m_database.remove(key);
    return Response::ok();
}

Response MessageHandler::Dispatcher::operator()(const ExistsRequestBody& body) {
    JSTINE_PROFILE_REGION("ExistsRequest");
    if (m_database.exists(Key{body.key})) {
        return Response::ok();
    }
    return Response::error(ErrorCode::notFound, "Key does not exist");
}

}  // namespace jstine
