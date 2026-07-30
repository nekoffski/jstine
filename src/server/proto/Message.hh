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
    ttl = 6,
    persist = 7,
    expire = 8,
};

enum class ResponseKind {
    ok = 0,
    error = 1,
};

enum class SetCondition : u8 {
    none = 0,
    nx = 1,
    xx = 2,
};

struct PingRequestBody {
    Bytes payload;
};

struct SetRequestBody {
    Bytes key;
    Bytes value;
    SetCondition condition = SetCondition::none;
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

struct TtlRequestBody {
    Bytes key;
};

struct PersistRequestBody {
    Bytes key;
};

struct ExpireRequestBody {
    Bytes key;
    u64 seconds;
};

using RequestBody = std::variant<
    PingRequestBody, SetRequestBody, GetRequestBody, DelRequestBody,
    ExistsRequestBody, TtlRequestBody, PersistRequestBody, ExpireRequestBody>;

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
    static Response ok(CBytesView payload = {});
};

}  // namespace jstine
