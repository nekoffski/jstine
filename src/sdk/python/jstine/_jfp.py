import struct
from enum import IntEnum

from ._codec import Codec
from ._proto import FieldType, RequestKind, ResponseKind
from .errors import ErrorCode

_FRAME_HEADER_FMT = "<II"
_FIELD_HEADER_FMT = "<BI"
_FRAME_HEADER_SIZE = 8
_FIELD_HEADER_SIZE = 5


def _pack_field(ftype: FieldType, data: bytes) -> bytes:
    return struct.pack(_FIELD_HEADER_FMT, int(ftype), len(data)) + data


def _pack_request(kind: RequestKind, fields: list[tuple[FieldType, bytes]]) -> bytes:
    fields_bytes = b"".join(_pack_field(ft, d) for ft, d in fields)
    payload = struct.pack("<I", int(kind)) + fields_bytes
    return struct.pack("<I", len(payload)) + payload


def _unpack_fields(data: bytes) -> list[tuple[FieldType, bytes]]:
    result = []
    pos = 0
    while pos < len(data):
        if len(data) - pos < _FIELD_HEADER_SIZE:
            raise ValueError("Truncated field header")
        ftype, fsize = struct.unpack_from(_FIELD_HEADER_FMT, data, pos)
        pos += _FIELD_HEADER_SIZE
        if pos + fsize > len(data):
            raise ValueError("Field data out of bounds")
        result.append((FieldType(ftype), data[pos: pos + fsize]))
        pos += fsize
    return result


class JFPCodec(Codec):
    def pack_ping(self, payload: bytes) -> bytes:
        fields = [(FieldType.payload, payload)] if payload else []
        return _pack_request(RequestKind.ping, fields)

    def unpack_response(self, data: bytes) -> bytes:
        from .client import JstineError

        payload_size, kind_raw = struct.unpack_from(_FRAME_HEADER_FMT, data, 0)
        fields_data = data[_FRAME_HEADER_SIZE: 4 + payload_size]
        kind = ResponseKind(kind_raw)
        fields = _unpack_fields(fields_data)

        if kind == ResponseKind.error:
            code_raw = next((d for t, d in fields if t ==
                            FieldType.error_code), None)
            msg_raw = next((d for t, d in fields if t ==
                           FieldType.error_message), None)
            code = int.from_bytes(code_raw, "little") if code_raw else 0
            message = msg_raw.decode(errors="replace") if msg_raw else ""
            raise JstineError(ErrorCode(code), message)

        return next((d for t, d in fields if t == FieldType.payload), b"")
