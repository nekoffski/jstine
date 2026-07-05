from __future__ import annotations

import asyncio
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import jstine
from jstine._jfp import JFPCodec
from jstine._proto import FieldType, RequestKind
from jstine.client import JstineError
from jstine.errors import ErrorCode


class JFPCodecTests(unittest.TestCase):
    def test_pack_request(self) -> None:
        codec = JFPCodec()
        data = codec.pack_set(b"key", b"value")

        payload_size, kind = struct.unpack_from("<II", data, 0)
        self.assertEqual(kind, int(RequestKind.set))
        self.assertEqual(payload_size, len(data) - 4)

    def test_unpack_ok_response(self) -> None:
        codec = JFPCodec()
        payload = b"hello"
        field = struct.pack("<BI", int(FieldType.payload), len(payload)) + payload
        data = struct.pack("<II", 4 + len(field), 0) + field

        self.assertEqual(codec.unpack_response(data), payload)

    def test_unpack_error_response(self) -> None:
        codec = JFPCodec()
        code = struct.pack("<I", int(ErrorCode.notFound))
        message = b"missing"
        code_field = struct.pack("<BI", int(FieldType.error_code), len(code)) + code
        msg_field = struct.pack("<BI", int(FieldType.error_message), len(message)) + message
        data = struct.pack("<II", 4 + len(code_field) + len(msg_field), 1) + code_field + msg_field

        with self.assertRaises(JstineError) as ctx:
            codec.unpack_response(data)
        self.assertEqual(ctx.exception.code, ErrorCode.notFound)
        self.assertEqual(str(ctx.exception), "missing")


class ClientBehaviorTests(unittest.TestCase):
    def test_get_maps_not_found_to_none(self) -> None:
        client = jstine.Client()
        client._request = lambda *_: (_ for _ in ()).throw(
            JstineError(ErrorCode.notFound, "missing")
        )

        self.assertIsNone(client.get("k"))

    def test_exists_maps_not_found_to_false(self) -> None:
        client = jstine.Client()
        client._request = lambda *_: (_ for _ in ()).throw(
            JstineError(ErrorCode.notFound, "missing")
        )

        self.assertFalse(client.exists("k"))

    def test_set_coerces_simple_types(self) -> None:
        client = jstine.Client()
        calls: list[tuple[RequestKind, list[tuple[FieldType, bytes]]]] = []

        def fake_request(kind: RequestKind, fields: list[tuple[FieldType, bytes]]) -> bytes:
            calls.append((kind, fields))
            return b""

        client._request = fake_request

        self.assertTrue(client.set("k", 42))
        self.assertEqual(
            calls,
            [
                (
                    RequestKind.set,
                    [
                        (FieldType.key, b"k"),
                        (FieldType.value, b"42"),
                    ],
                )
            ],
        )


class AsyncClientBehaviorTests(unittest.IsolatedAsyncioTestCase):
    async def test_delete_maps_not_found_to_false(self) -> None:
        client = jstine.AsyncClient()

        async def fake_request(*_args, **_kwargs):
            raise JstineError(ErrorCode.notFound, "missing")

        client._request = fake_request  # type: ignore[method-assign]

        self.assertFalse(await client.delete("k"))


if __name__ == "__main__":
    unittest.main()
