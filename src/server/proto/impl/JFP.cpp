#include "JFP.hh"

#include <cstring>

#include "api/Message.hh"
#include "core/Error.hh"

namespace jstine {

namespace {

// Frame header: size prefix + request/response kind.
// The size prefix counts everything after it, including the kind field.
constexpr u64 frameHeaderSize = 8;
// Field header: type tag + field byte count.
constexpr u64 fieldHeaderSize = 5;

using FieldMap = std::vector<std::pair<JFPFieldType, Bytes>>;

u32 readU32(const Byte* p) {
    u32 v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

void writeU32(Byte* p, u32 v) { std::memcpy(p, &v, sizeof(v)); }

void appendField(Bytes& out, JFPFieldType type, std::span<const Byte> data) {
    out.push_back(static_cast<u8>(type));
    u32 size = static_cast<u32>(data.size());
    const auto* sb = reinterpret_cast<const Byte*>(&size);
    out.insert(out.end(), sb, sb + sizeof(size));
    out.insert(out.end(), data.begin(), data.end());
}

const Bytes* field(const FieldMap& fields, JFPFieldType type) {
    for (const auto& [t, d] : fields) {
        if (t == type) {
            return &d;
        }
    }
    return nullptr;
}

Result<const Bytes*> require(
    const FieldMap& fields, JFPFieldType type, std::string_view name
) {
    const auto* f = field(fields, type);
    if (not f) {
        return Error::unexpected(ErrorCode::badInput, "{}", name);
    }
    return f;
}

Result<FieldMap> parseFields(std::span<const Byte> data) {
    FieldMap fields;
    const Byte* pos = data.data();
    const Byte* end = pos + data.size();

    while (pos < end) {
        // The parser is strict: every field must have a complete header.
        if (static_cast<u64>(end - pos) < fieldHeaderSize) {
            return Error::unexpected(
                ErrorCode::badInput, "Truncated field header"
            );
        }

        const auto ftype = static_cast<JFPFieldType>(*pos);
        const u32 fsize = readU32(pos + 1);
        pos += fieldHeaderSize;

        // Field payloads must stay within the current frame boundary.
        if (pos + fsize > end) {
            return Error::unexpected(
                ErrorCode::badInput, "Field data out of bounds"
            );
        }

        fields.emplace_back(ftype, Bytes(pos, pos + fsize));
        pos += fsize;
    }
    return fields;
}

Result<Request> buildRequest(u32 kindRaw, const FieldMap& fields) {
    const auto req = [&](JFPFieldType type, std::string_view name) {
        return require(fields, type, name);
    };

    // Each request kind maps to a required set of fields.
    // We validate those requirements here instead of later in the handler so
    // malformed frames fail fast at the protocol boundary.
    switch (static_cast<RequestKind>(kindRaw)) {
        case RequestKind::ping: {
            const auto* p = field(fields, JFPFieldType::payload);
            return Request{
                RequestKind::ping, PingRequestBody{p ? *p : Bytes{}}
            };
        }
        case RequestKind::get: {
            auto k = req(JFPFieldType::key, "get: missing key");
            if (not k) {
                return std::unexpected(k.error());
            }
            return Request{RequestKind::get, GetRequestBody{**k}};
        }
        case RequestKind::set: {
            auto k = req(JFPFieldType::key, "set: missing key");
            if (not k) {
                return std::unexpected(k.error());
            }
            auto v = req(JFPFieldType::value, "set: missing value");
            if (not v) {
                return std::unexpected(v.error());
            }
            return Request{RequestKind::set, SetRequestBody{**k, **v}};
        }
        case RequestKind::del: {
            auto k = req(JFPFieldType::key, "del: missing key");
            if (not k) {
                return std::unexpected(k.error());
            }
            return Request{RequestKind::del, DelRequestBody{**k}};
        }
        case RequestKind::exists: {
            auto k = req(JFPFieldType::key, "exists: missing key");
            if (not k) {
                return std::unexpected(k.error());
            }
            return Request{RequestKind::exists, ExistsRequestBody{**k}};
        }
        default:
            return Error::unexpected(
                ErrorCode::badInput, "Unknown request kind: {}", kindRaw
            );
    }
}

void appendBodyFields(Bytes& out, const OkResponseBody& body) {
    // OK responses omit the payload field when there is nothing to return.
    if (not body.payload.empty()) {
        appendField(out, JFPFieldType::payload, body.payload);
    }
}

void appendBodyFields(Bytes& out, const ErrorResponseBody& body) {
    // Error responses always include the code so the client can classify the
    // failure even if the message is empty.
    Bytes codeBytes(4);
    writeU32(codeBytes.data(), body.code);
    appendField(out, JFPFieldType::errorCode, codeBytes);
    if (not body.message.empty()) {
        appendField(out, JFPFieldType::errorMessage, body.message);
    }
}

Bytes serializeBody(const Response& response) {
    Bytes out;
    std::visit(
        [&](const auto& body) { appendBodyFields(out, body); }, response.body
    );
    return out;
}

}  // namespace

void JFPRequestDecoder::feed(std::span<const Byte> bytes) {
    m_buf.insert(m_buf.end(), bytes.begin(), bytes.end());
}

Result<Request> JFPRequestDecoder::decode() {
    // Not enough data for the fixed frame header yet.
    if (m_buf.size() < frameHeaderSize) {
        return Error::unexpected(
            ErrorCode::requestNotReady, "Incomplete frame header"
        );
    }

    // `payloadSize` is everything after the size prefix itself.
    const u32 payloadSize = readU32(m_buf.data());
    const u64 totalSize = 4 + static_cast<u64>(payloadSize);

    // We have a complete header, but not necessarily a complete frame.
    if (m_buf.size() < totalSize) {
        return Error::unexpected(
            ErrorCode::requestNotReady, "Incomplete frame"
        );
    }

    const u32 kindRaw = readU32(m_buf.data() + 4);

    // Parse only the current frame body. Any bytes after `totalSize` remain in
    // the buffer for a later decode call.
    auto fields = parseFields(
        {m_buf.data() + frameHeaderSize, totalSize - frameHeaderSize}
    );
    if (not fields) {
        return Error::unexpected(fields.error());
    }

    m_buf.erase(
        m_buf.begin(), m_buf.begin() + static_cast<ptrdiff_t>(totalSize)
    );

    return buildRequest(kindRaw, *fields);
}

Result<u64> JFPResponseEncoder::encode(
    const Response& response, std::span<Byte> out
) {
    const Bytes fields = serializeBody(response);
    const u32 payloadSize = static_cast<u32>(4 + fields.size());
    const u64 totalSize = 4 + payloadSize;

    // The encoder writes the frame in-place into the caller-provided buffer.
    // If the buffer is too small we fail instead of resizing implicitly, so
    // the caller controls allocation and frame lifetime.
    if (out.size() < totalSize) {
        return Error::unexpected(
            ErrorCode::badInput, "Output buffer too small: need {}, have {}",
            totalSize, out.size()
        );
    }

    Byte* p = out.data();
    writeU32(p, payloadSize);
    p += 4;
    writeU32(p, static_cast<u32>(response.kind));
    p += 4;
    std::memcpy(p, fields.data(), fields.size());

    return totalSize;
}

}  // namespace jstine
