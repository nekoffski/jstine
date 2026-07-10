from __future__ import annotations

import socket
import struct
import time
from contextlib import suppress

from jstine._proto import FieldType, HEADER_SIZE, Protocol, RequestKind, ResponseKind

_HANDSHAKE_FMT = "<II8s"
_FRAME_HEADER_FMT = "<II"
_FIELD_HEADER_FMT = "<BI"
_FRAME_HEADER_SIZE = 8
_REQUEST_MAGIC = 0xDEADBEEF
_RESPONSE_MAGIC = 0xBEEFDEAD


def pack_handshake(
    protocol: Protocol = Protocol.jfp,
    magic: int = _REQUEST_MAGIC,
) -> bytes:
    return struct.pack(_HANDSHAKE_FMT, int(protocol), magic, bytes(8))


def pack_field(field_type: FieldType, payload: bytes) -> bytes:
    return struct.pack(_FIELD_HEADER_FMT, int(field_type), len(payload)) + payload


def pack_request(
    kind: RequestKind, fields: list[tuple[FieldType, bytes]] | None = None
) -> bytes:
    fields = fields or []
    payload = struct.pack("<I", int(kind))
    payload += b"".join(pack_field(field_type, value) for field_type, value in fields)
    return struct.pack("<I", len(payload)) + payload


def unpack_response(data: bytes) -> tuple[ResponseKind, list[tuple[FieldType, bytes]]]:
    payload_size, kind_raw = struct.unpack_from(_FRAME_HEADER_FMT, data, 0)
    if payload_size < 4:
        raise ValueError(f"Invalid payload size: {payload_size}")

    fields_data = data[8: 4 + payload_size]
    fields: list[tuple[FieldType, bytes]] = []
    pos = 0
    while pos < len(fields_data):
        if len(fields_data) - pos < 5:
            raise ValueError("Truncated field header")
        field_raw, field_size = struct.unpack_from(_FIELD_HEADER_FMT, fields_data, pos)
        pos += 5
        if pos + field_size > len(fields_data):
            raise ValueError("Field data out of bounds")
        fields.append((FieldType(field_raw), fields_data[pos: pos + field_size]))
        pos += field_size

    return ResponseKind(kind_raw), fields


class RawJFPWire:
    def __init__(self, testcase, host: str = "127.0.0.1") -> None:
        self._testcase = testcase
        self._host = host
        self._port = testcase.config.server.port
        self._sock: socket.socket | None = None
        testcase.addCleanup(self.close)

    def open(self) -> "RawJFPWire":
        if self._sock is not None:
            self.close()

        self._sock = socket.create_connection((self._host, self._port), timeout=2.0)
        self._sock.settimeout(2.0)
        return self

    def close(self) -> None:
        if self._sock is None:
            return

        with suppress(OSError):
            self._sock.close()
        self._sock = None

    def send(self, data: bytes) -> None:
        assert self._sock is not None
        self._sock.sendall(data)

    def send_chunks(self, *chunks: bytes) -> None:
        assert self._sock is not None
        for chunk in chunks:
            self._sock.sendall(chunk)

    def recv_exact(self, size: int) -> bytes:
        assert self._sock is not None
        data = bytearray()
        while len(data) < size:
            chunk = self._sock.recv(size - len(data))
            if not chunk:
                raise ConnectionError("Connection closed by peer")
            data.extend(chunk)
        return bytes(data)

    def read_handshake(self) -> Protocol:
        header = self.recv_exact(HEADER_SIZE)
        protocol, magic, _ = struct.unpack(_HANDSHAKE_FMT, header)
        if magic != _RESPONSE_MAGIC:
            raise ValueError(f"Unexpected handshake magic: 0x{magic:08X}")
        return Protocol(protocol)

    def handshake(
        self,
        protocol: Protocol = Protocol.jfp,
        magic: int = _REQUEST_MAGIC,
    ) -> Protocol:
        self.send(pack_handshake(protocol=protocol, magic=magic))
        return self.read_handshake()

    def request(
        self, kind: RequestKind, fields: list[tuple[FieldType, bytes]] | None = None
    ) -> tuple[ResponseKind, list[tuple[FieldType, bytes]]]:
        self.send(pack_request(kind, fields))
        header = self.recv_exact(_FRAME_HEADER_SIZE)
        payload_size, _ = struct.unpack(_FRAME_HEADER_FMT, header)
        body = self.recv_exact(payload_size - 4) if payload_size > 4 else b""
        return unpack_response(header + body)

    def request_chunks(
        self,
        header_chunks: tuple[bytes, ...],
        body_chunks: tuple[bytes, ...] = (),
    ) -> tuple[ResponseKind, list[tuple[FieldType, bytes]]]:
        self.send_chunks(*header_chunks, *body_chunks)
        header = self.recv_exact(_FRAME_HEADER_SIZE)
        payload_size, _ = struct.unpack(_FRAME_HEADER_FMT, header)
        body = self.recv_exact(payload_size - 4) if payload_size > 4 else b""
        return unpack_response(header + body)

    def wait_for_close(self, attempts: int = 20) -> None:
        assert self._sock is not None
        for _ in range(attempts):
            try:
                data = self._sock.recv(1)
            except (ConnectionResetError, OSError):
                return
            if data == b"":
                return
            time.sleep(0.05)
        raise AssertionError("socket stayed open")
