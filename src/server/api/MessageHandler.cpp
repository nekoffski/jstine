#include "MessageHandler.hh"

#include "core/Functional.hh"

namespace jstine {

Response MessageHandler::onRequest(const Request& request) {
    return std::visit(m_dispatcher, request.body);
}

Response MessageHandler::Dispatcher::operator()(const PingRequestBody& body) {
    return Response::ok(body.payload);
}

Response MessageHandler::Dispatcher::operator()(const SetRequestBody& body) {
    return Response::error(
        ErrorCode::notImplementedYet, "Set request not implemented yet"
    );
}

Response MessageHandler::Dispatcher::operator()(const GetRequestBody& body) {
    return Response::error(
        ErrorCode::notImplementedYet, "Get request not implemented yet"
    );
}

Response MessageHandler::Dispatcher::operator()(const DelRequestBody& body) {
    return Response::error(
        ErrorCode::notImplementedYet, "Del request not implemented yet"
    );
}

Response MessageHandler::Dispatcher::operator()(const ExistsRequestBody& body) {
    return Response::error(
        ErrorCode::notImplementedYet, "Exists request not implemented yet"
    );
}

}  // namespace jstine
