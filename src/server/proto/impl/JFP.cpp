#include "JFP.hh"

#include <cstring>

#include "api/Message.hh"
#include "core/Error.hh"

namespace jstine {

namespace {

constexpr u64 frameHeaderSize = 8;  // u32 payload_size + u32 kind
constexpr u64 fieldHeaderSize = 5;  // u8 type + u32 size

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
    out.insert(out.end(), sb, sb + 4);
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
        if (static_cast<u64>(end - pos) < fieldHeaderSize) {
            return Error::unexpected(
                ErrorCode::badInput, "Truncated field header"
            );
        }

        const auto ftype = static_cast<JFPFieldType>(*pos);
        const u32 fsize = readU32(pos + 1);
        pos += fieldHeaderSize;

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
    if (not body.payload.empty()) {
        appendField(out, JFPFieldType::payload, body.payload);
    }
}

void appendBodyFields(Bytes& out, const ErrorResponseBody& body) {
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
    if (m_buf.size() < frameHeaderSize) {
        return Error::unexpected(
            ErrorCode::requestNotReady, "Incomplete frame header"
        );
    }

    const u32 payloadSize = readU32(m_buf.data());
    const u64 totalSize = 4 + static_cast<u64>(payloadSize);

    if (m_buf.size() < totalSize) {
        return Error::unexpected(
            ErrorCode::requestNotReady, "Incomplete frame"
        );
    }

    const u32 kindRaw = readU32(m_buf.data() + 4);

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
