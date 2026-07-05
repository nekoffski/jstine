#pragma once

#include <variant>

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

enum class RequestKind {
    ping = 1,
    set = 2,
    get = 3,
    del = 4,
    exists = 5,
};

enum class ResponseKind {
    ok = 0,
    error = 1,
};

struct PingRequestBody {
    Bytes payload;
};

struct SetRequestBody {
    Bytes key;
    Bytes value;
};

struct GetRequestBody {
    Bytes key;
};

struct DelRequestBody {
    Bytes key;
};

struct ExistsRequestBody {
    Bytes key;
};

using RequestBody = std::variant<
    PingRequestBody, SetRequestBody, GetRequestBody, DelRequestBody,
    ExistsRequestBody>;

struct OkResponseBody {
    Bytes payload;
};

struct ErrorResponseBody {
    u32 code;
    Bytes message;
};

using ResponseBody = std::variant<OkResponseBody, ErrorResponseBody>;

struct Request {
    RequestKind kind;
    RequestBody body;
};

struct Response {
    ResponseKind kind;
    ResponseBody body;

    static Response error(const Error& err);
    static Response error(ErrorCode code, const std::string& message);
};

}  // namespace jstine
