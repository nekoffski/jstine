import socket
import struct

from ._codec import Codec, make_codec
from ._proto import HEADER_SIZE, Protocol, pack_handshake, unpack_handshake
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
        assert self._codec is not None
        self._send(self._codec.pack_ping(payload))
        return self._recv_response()

    def _handshake(self) -> None:
        self._send(pack_handshake(self._protocol))
        self._protocol = unpack_handshake(self._recv_exact(HEADER_SIZE))
        self._codec = make_codec(self._protocol)

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
