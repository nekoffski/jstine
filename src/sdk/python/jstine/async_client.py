import asyncio
import struct

from ._codec import Codec, make_codec
from ._proto import HEADER_SIZE, Protocol, pack_handshake, unpack_handshake
from .client import JstineError

_FRAME_HEADER_SIZE = 8
_FRAME_HEADER_FMT = "<II"


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
        self._codec: Codec | None = None

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
        assert self._codec is not None
        self._send(self._codec.pack_ping(payload))
        await self._flush()
        return await self._recv_response()

    async def _handshake(self) -> None:
        self._send(pack_handshake(self._protocol))
        await self._flush()
        self._protocol = unpack_handshake(await self._recv_exact(HEADER_SIZE))
        self._codec = make_codec(self._protocol)

    def _send(self, data: bytes) -> None:
        assert self._writer is not None
        self._writer.write(data)

    async def _flush(self) -> None:
        assert self._writer is not None
        await self._writer.drain()

    async def _recv_exact(self, n: int) -> bytes:
        assert self._reader is not None
        return await self._reader.readexactly(n)

    async def _recv_response(self) -> bytes:
        assert self._codec is not None
        header = await self._recv_exact(_FRAME_HEADER_SIZE)
        payload_size, _ = struct.unpack(_FRAME_HEADER_FMT, header)
        rest = await self._recv_exact(payload_size - 4) if payload_size > 4 else b""
        return self._codec.unpack_response(header + rest)
