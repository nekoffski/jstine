import struct
from enum import IntEnum

# Handshake header: u32 protocol + u32 magic + 8 bytes padding = 16 bytes
_HEADER_FMT = "<II8s"
HEADER_SIZE = struct.calcsize(_HEADER_FMT)

MAGIC_REQUEST = 0xDEADBEEF
MAGIC_RESPONSE = 0xBEEFDEAD

# JFP frame: u32 kind + u32 payload_length
_FRAME_FMT = "<II"
FRAME_HEADER_SIZE = struct.calcsize(_FRAME_FMT)


class Protocol(IntEnum):
    rsp = 1
    jfp = 2


class RequestKind(IntEnum):
    ping = 1
    set = 2
    get = 3
    delete = 4
    exists = 5


class ResponseKind(IntEnum):
    ok = 0
    error = 1


def pack_handshake(protocol: Protocol) -> bytes:
    return struct.pack(_HEADER_FMT, int(protocol), MAGIC_REQUEST, bytes(8))


def unpack_handshake(data: bytes) -> Protocol:
    if len(data) < HEADER_SIZE:
        raise ValueError(f"Handshake too short: {len(data)} < {HEADER_SIZE}")
    protocol, magic, _ = struct.unpack(_HEADER_FMT, data[:HEADER_SIZE])
    if magic != MAGIC_RESPONSE:
        raise ValueError(f"Unexpected handshake magic: 0x{magic:08X}")
    return Protocol(protocol)


def pack_request(kind: RequestKind, payload: bytes = b"") -> bytes:
    return struct.pack(_FRAME_FMT, int(kind), len(payload)) + payload


def unpack_response_header(data: bytes) -> tuple[ResponseKind, int]:
    if len(data) < FRAME_HEADER_SIZE:
        raise ValueError(f"Frame header too short: {len(data)}")
    kind, length = struct.unpack(_FRAME_FMT, data[:FRAME_HEADER_SIZE])
    return ResponseKind(kind), length
