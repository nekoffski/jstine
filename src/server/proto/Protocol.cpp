#include "Protocol.hh"

namespace jstine {

Str protocolToStr(Protocol protocol) {
    switch (protocol) {
        case Protocol::rsp:
            return "rsp";
        case Protocol::jfp:
            return "jfp";
    }
    return "unknown";
}

bool ProtocolHeader::protocolValid() const {
    return (protocol == Protocol::rsp || protocol == Protocol::jfp);
}

}  // namespace jstine
