import socket

from ._proto import (
    FRAME_HEADER_SIZE,
    HEADER_SIZE,
    Protocol,
    RequestKind,
    ResponseKind,
    pack_handshake,
    pack_request,
    unpack_handshake,
    unpack_response_header,
)
from .errors import ErrorCode


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

    def connect(self) -> None:
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.connect((self._host, self._port))
        self._handshake()

    def close(self) -> None:
        if self._sock:
            self._sock.close()
            self._sock = None

    def __enter__(self) -> "Client":
        self.connect()
        return self

    def __exit__(self, *_) -> None:
        self.close()

    def ping(self, payload: bytes = b"") -> bytes:
        self._send(pack_request(RequestKind.ping, payload))
        return self._recv_response()

    def _handshake(self) -> None:
        self._send(pack_handshake(self._protocol))
        data = self._recv_exact(HEADER_SIZE)
        self._protocol = unpack_handshake(data)

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
        header = self._recv_exact(FRAME_HEADER_SIZE)
        kind, length = unpack_response_header(header)
        payload = self._recv_exact(length) if length else b""
        if kind == ResponseKind.error:
            code = int.from_bytes(payload[:4], "little")
            message = payload[8:].decode(errors="replace")
            raise JstineError(ErrorCode(code), message)
        return payload
