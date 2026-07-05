import asyncio

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
from .client import JstineError
from .errors import ErrorCode


class AsyncClient:
    def __init__(
        self,
        host: str = "127.0.0.1",
        port: int = 9991,
        protocol: Protocol = Protocol.jfp,
    ):
        self._host = host
        self._port = port
        self._protocol = protocol
        self._reader: asyncio.StreamReader | None = None
        self._writer: asyncio.StreamWriter | None = None

    async def connect(self) -> None:
        self._reader, self._writer = await asyncio.open_connection(
            self._host, self._port
        )
        await self._handshake()

    async def close(self) -> None:
        if self._writer:
            self._writer.close()
            await self._writer.wait_closed()
            self._reader = None
            self._writer = None

    async def __aenter__(self) -> "AsyncClient":
        await self.connect()
        return self

    async def __aexit__(self, *_) -> None:
        await self.close()

    async def ping(self, payload: bytes = b"") -> bytes:
        self._send(pack_request(RequestKind.ping, payload))
        await self._flush()
        return await self._recv_response()

    async def _handshake(self) -> None:
        self._send(pack_handshake(self._protocol))
        await self._flush()
        data = await self._recv_exact(HEADER_SIZE)
        self._protocol = unpack_handshake(data)

    def _send(self, data: bytes) -> None:
        assert self._writer is not None
        self._writer.write(data)

    async def _flush(self) -> None:
        assert self._writer is not None
        await self._writer.drain()

    async def _recv_exact(self, n: int) -> bytes:
        assert self._reader is not None
        data = await self._reader.readexactly(n)
        return data

    async def _recv_response(self) -> bytes:
        header = await self._recv_exact(FRAME_HEADER_SIZE)
        kind, length = unpack_response_header(header)
        payload = await self._recv_exact(length) if length else b""
        if kind == ResponseKind.error:
            code = int.from_bytes(payload[:4], "little")
            message = payload[8:].decode(errors="replace")
            raise JstineError(ErrorCode(code), message)
        return payload
