import struct
from enum import IntEnum

# Handshake header: u32 protocol + u32 magic + 8 bytes padding = 16 bytes
_HEADER_FMT = "<II8s"
HEADER_SIZE = struct.calcsize(_HEADER_FMT)

MAGIC_REQUEST = 0xDEADBEEF
MAGIC_RESPONSE = 0xBEEFDEAD


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


# Field type identifiers — must match JFPFieldType in JFP.hh
class FieldType(IntEnum):
    payload = 1  # PingRequest payload / OkResponse payload
    key = 2  # Get/Set/Del/Exists key
    value = 3  # Set value
    error_code = 4  # ErrorResponse code (u32 LE)
    error_message = 5  # ErrorResponse message (utf-8 bytes)


def pack_handshake(protocol: Protocol) -> bytes:
    return struct.pack(_HEADER_FMT, int(protocol), MAGIC_REQUEST, bytes(8))


def unpack_handshake(data: bytes) -> Protocol:
    if len(data) < HEADER_SIZE:
        raise ValueError(f"Handshake too short: {len(data)} < {HEADER_SIZE}")
    protocol, magic, _ = struct.unpack(_HEADER_FMT, data[:HEADER_SIZE])
    if magic != MAGIC_RESPONSE:
        raise ValueError(f"Unexpected handshake magic: 0x{magic:08X}")
    return Protocol(protocol)
