#include "Message.hh"

#include "core/Log.hh"

namespace jstine {

Response Response::error(const Error& err) {
    return Response{
        ResponseKind::error,
        ErrorResponseBody{
            fmt::underlying(err.code()),
            Bytes(err.message().begin(), err.message().end())
        }
    };
}

Response Response::error(ErrorCode code, const std::string& message) {
    return Response{
        ResponseKind::error,
        ErrorResponseBody{
            fmt::underlying(code), Bytes(message.begin(), message.end())
        }
    };
}

Response Response::ok(const Bytes& payload) {
    return Response{ResponseKind::ok, OkResponseBody{payload}};
}

}  // namespace jstine
