#pragma once

namespace jstine {

enum class RequestKind {
    noop = 0,
    health = 1,
    put = 2,
    insert = 3,
    get = 4,
    exists = 5,
    remove = 6,
};

enum class ResponseKind {
    ok = 0,
    error = 1,
};

struct Request {
    RequestKind kind;
};

struct Response {
    ResponseKind kind;
};

}  // namespace jstine
