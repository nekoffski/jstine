#include "JFP.hh"

namespace jstine {

Result<u64> JFPResponseEncoder::encode(
    const Response& response, std::span<Byte> bytes
) {
    return Result<u64>();
}
Result<Request> JFPRequestDecoder::decode() { return Result<Request>(); }

}  // namespace jstine
