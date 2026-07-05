from __future__ import annotations

import asyncio
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
from .client import JstineError
from .errors import ErrorCode

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
        if self._writer is not None:
            await self.close()
        try:
            self._reader, self._writer = await asyncio.open_connection(
                self._host, self._port
            )
            await self._handshake()
        except Exception:
            await self.close()
            raise

    async def close(self) -> None:
        if self._writer is not None:
            self._writer.close()
            await self._writer.wait_closed()
            self._reader = None
            self._writer = None
        self._codec = None

    async def __aenter__(self) -> "AsyncClient":
        await self.connect()
        return self

    async def __aexit__(self, *_) -> None:
        await self.close()

    async def ping(
        self,
        payload: bytes | bytearray | memoryview | str | int | float = b"",
    ) -> bytes:
        payload_bytes = coerce_bytes(payload)
        return await self._request(
            RequestKind.ping,
            [(FieldType.payload, payload_bytes)] if payload_bytes else [],
        )

    async def set(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
        value: bytes | bytearray | memoryview | str | int | float,
    ) -> bool:
        await self._request(
            RequestKind.set,
            [
                (FieldType.key, coerce_bytes(key)),
                (FieldType.value, coerce_bytes(value)),
            ],
        )
        return True

    async def get(
        self, key: bytes | bytearray | memoryview | str | int | float
    ) -> bytes | None:
        try:
            return await self._request(
                RequestKind.get, [(FieldType.key, coerce_bytes(key))]
            )
        except JstineError as exc:
            if exc.code == ErrorCode.notFound:
                return None
            raise

    async def delete(
        self, key: bytes | bytearray | memoryview | str | int | float
    ) -> bool:
        try:
            await self._request(
                RequestKind.delete, [(FieldType.key, coerce_bytes(key))]
            )
            return True
        except JstineError as exc:
            if exc.code == ErrorCode.notFound:
                return False
            raise

    async def del_(
        self, key: bytes | bytearray | memoryview | str | int | float
    ) -> bool:
        return await self.delete(key)

    async def exists(
        self, key: bytes | bytearray | memoryview | str | int | float
    ) -> bool:
        try:
            await self._request(
                RequestKind.exists, [(FieldType.key, coerce_bytes(key))]
            )
            return True
        except JstineError as exc:
            if exc.code == ErrorCode.notFound:
                return False
            raise

    async def _handshake(self) -> None:
        self._send(pack_handshake(self._protocol))
        await self._flush()
        self._protocol = unpack_handshake(await self._recv_exact(HEADER_SIZE))
        self._codec = make_codec(self._protocol)

    async def _request(
        self, kind: RequestKind, fields: list[tuple[FieldType, bytes]]
    ) -> bytes:
        assert self._codec is not None
        self._send(self._codec.pack_request(kind, fields))
        await self._flush()
        return await self._recv_response()

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
