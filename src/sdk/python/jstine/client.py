from __future__ import annotations

import socket
import struct

from ._codec import Codec, make_codec
from ._common import coerce_bytes
from ._proto import (
    FieldType,
    HEADER_SIZE,
    Protocol,
    RequestKind,
    pack_handshake,
    unpack_handshake,
)
from .errors import ErrorCode

_FRAME_HEADER_SIZE = 8
_FRAME_HEADER_FMT = "<II"


class JstineError(Exception):
    def __init__(self, code: ErrorCode, message: str):
        super().__init__(message)
        self.code = code


class Client:
    def __init__(
        self,
        host: str = "127.0.0.1",
        port: int = 9991,
        protocol: Protocol = Protocol.jfp,
    ):
        self._host = host
        self._port = port
        self._protocol = protocol
        self._sock: socket.socket | None = None
        self._codec: Codec | None = None

    def connect(self) -> None:
        if self._sock is not None:
            self.close()
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self._sock.connect((self._host, self._port))
            self._handshake()
        except Exception:
            self.close()
            raise

    def close(self) -> None:
        if self._sock is not None:
            self._sock.close()
            self._sock = None
        self._codec = None

    def __enter__(self) -> "Client":
        self.connect()
        return self

    def __exit__(self, *_) -> None:
        self.close()

    def ping(
        self,
        payload: bytes | bytearray | memoryview | str | int | float = b"",
    ) -> bytes:
        payload_bytes = coerce_bytes(payload)
        return self._request(
            RequestKind.ping,
            [(FieldType.payload, payload_bytes)] if payload_bytes else [],
        )

    def set(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
        value: bytes | bytearray | memoryview | str | int | float,
    ) -> bool:
        self._request(
            RequestKind.set,
            [
                (FieldType.key, coerce_bytes(key)),
                (FieldType.value, coerce_bytes(value)),
            ],
        )
        return True

    def get(
        self, key: bytes | bytearray | memoryview | str | int | float
    ) -> bytes | None:
        try:
            return self._request(
                RequestKind.get, [(FieldType.key, coerce_bytes(key))]
            )
        except JstineError as exc:
            if exc.code == ErrorCode.notFound:
                return None
            raise

    def delete(
        self, key: bytes | bytearray | memoryview | str | int | float
    ) -> bool:
        try:
            self._request(
                RequestKind.delete, [(FieldType.key, coerce_bytes(key))]
            )
            return True
        except JstineError as exc:
            if exc.code == ErrorCode.notFound:
                return False
            raise

    def del_(
        self, key: bytes | bytearray | memoryview | str | int | float
    ) -> bool:
        return self.delete(key)

    def exists(
        self, key: bytes | bytearray | memoryview | str | int | float
    ) -> bool:
        try:
            self._request(
                RequestKind.exists, [(FieldType.key, coerce_bytes(key))]
            )
            return True
        except JstineError as exc:
            if exc.code == ErrorCode.notFound:
                return False
            raise

    def _handshake(self) -> None:
        self._send(pack_handshake(self._protocol))
        self._protocol = unpack_handshake(self._recv_exact(HEADER_SIZE))
        self._codec = make_codec(self._protocol)

    def _request(
        self, kind: RequestKind, fields: list[tuple[FieldType, bytes]]
    ) -> bytes:
        assert self._codec is not None
        self._send(self._codec.pack_request(kind, fields))
        return self._recv_response()

    def _send(self, data: bytes) -> None:
        assert self._sock is not None
        self._sock.sendall(data)

    def _recv_exact(self, n: int) -> bytes:
        assert self._sock is not None
        buf = bytearray()
        while len(buf) < n:
            chunk = self._sock.recv(n - len(buf))
            if not chunk:
                raise ConnectionError("Connection closed by server")
            buf += chunk
        return bytes(buf)

    def _recv_response(self) -> bytes:
        assert self._codec is not None
        header = self._recv_exact(_FRAME_HEADER_SIZE)
        payload_size, _ = struct.unpack(_FRAME_HEADER_FMT, header)
        rest = self._recv_exact(payload_size - 4) if payload_size > 4 else b""
        return self._codec.unpack_response(header + rest)
