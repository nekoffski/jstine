from __future__ import annotations

import asyncio
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import jstine
from jstine._jfp import JFPCodec
from jstine._proto import FieldType, RequestKind, SetCondition
from jstine.client import JstineError
from jstine.errors import ErrorCode


class JFPCodecTests(unittest.TestCase):
    def test_pack_request(self) -> None:
        codec = JFPCodec()
        data = codec.pack_set(b"key", b"value")

        payload_size, kind = struct.unpack_from("<II", data, 0)
        self.assertEqual(kind, int(RequestKind.set))
        self.assertEqual(payload_size, len(data) - 4)

    def test_pack_new_request_types(self) -> None:
        codec = JFPCodec()

        set_nx = codec.pack_set(b"key", b"value", SetCondition.nx)
        ttl = codec.pack_ttl(b"key")
        persist = codec.pack_persist(b"key")
        expire = codec.pack_expire(b"key", 42)

        self.assertEqual(
            struct.unpack_from("<I", set_nx, 4)[0],
            int(RequestKind.set),
        )
        self.assertIn(
            struct.pack("<BI", int(FieldType.set_condition), 1)
            + bytes([SetCondition.nx]),
            set_nx,
        )
        self.assertEqual(
            struct.unpack_from("<I", ttl, 4)[0],
            int(RequestKind.ttl),
        )
        self.assertEqual(
            struct.unpack_from("<I", persist, 4)[0],
            int(RequestKind.persist),
        )
        self.assertEqual(
            struct.unpack_from("<I", expire, 4)[0],
            int(RequestKind.expire),
        )
        self.assertIn(
            struct.pack("<BIQ", int(FieldType.seconds), 8, 42),
            expire,
        )

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

    def test_setnx_and_setxx_encode_conditions(self) -> None:
        client = jstine.Client()
        calls: list[tuple[RequestKind, list[tuple[FieldType, bytes]]]] = []
        client._request = lambda kind, fields: calls.append((kind, fields)) or b""

        self.assertTrue(client.setnx("nx-key", "value"))
        self.assertTrue(client.setxx("xx-key", "value"))

        self.assertEqual(
            calls[0][1][-1],
            (FieldType.set_condition, bytes([SetCondition.nx])),
        )
        self.assertEqual(
            calls[1][1][-1],
            (FieldType.set_condition, bytes([SetCondition.xx])),
        )

    def test_ttl_decodes_seconds_and_persistent_value(self) -> None:
        client = jstine.Client()
        responses = iter([struct.pack("<Q", 42), b""])
        client._request = lambda *_: next(responses)

        self.assertEqual(client.ttl("expiring"), 42)
        self.assertIsNone(client.ttl("persistent"))

    def test_ttl_preserves_not_found_error(self) -> None:
        client = jstine.Client()
        client._request = lambda *_: (_ for _ in ()).throw(
            JstineError(ErrorCode.notFound, "missing")
        )

        with self.assertRaises(JstineError) as ctx:
            client.ttl("missing")
        self.assertEqual(ctx.exception.code, ErrorCode.notFound)

    def test_ttl_rejects_malformed_response(self) -> None:
        client = jstine.Client()
        client._request = lambda *_: b"\x01"

        with self.assertRaisesRegex(ValueError, "Invalid TTL response size"):
            client.ttl("key")

    def test_persist_encodes_request(self) -> None:
        client = jstine.Client()
        calls: list[tuple[RequestKind, list[tuple[FieldType, bytes]]]] = []
        client._request = lambda kind, fields: calls.append((kind, fields)) or b""

        self.assertTrue(client.persist("key"))
        self.assertEqual(
            calls,
            [(RequestKind.persist, [(FieldType.key, b"key")])],
        )

    def test_expire_encodes_u64_seconds(self) -> None:
        client = jstine.Client()
        calls: list[tuple[RequestKind, list[tuple[FieldType, bytes]]]] = []
        client._request = lambda kind, fields: calls.append((kind, fields)) or b""

        self.assertTrue(client.expire("key", 42))
        self.assertEqual(
            calls,
            [
                (
                    RequestKind.expire,
                    [
                        (FieldType.key, b"key"),
                        (FieldType.seconds, struct.pack("<Q", 42)),
                    ],
                )
            ],
        )

    def test_expire_rejects_invalid_seconds(self) -> None:
        client = jstine.Client()

        for value in (-1, 2**64):
            with self.subTest(value=value):
                with self.assertRaises(ValueError):
                    client.expire("key", value)
        with self.assertRaises(TypeError):
            client.expire("key", True)


class AsyncClientBehaviorTests(unittest.IsolatedAsyncioTestCase):
    async def test_delete_maps_not_found_to_false(self) -> None:
        client = jstine.AsyncClient()

        async def fake_request(*_args, **_kwargs):
            raise JstineError(ErrorCode.notFound, "missing")

        client._request = fake_request  # type: ignore[method-assign]

        self.assertFalse(await client.delete("k"))

    async def test_expiration_and_conditional_set_requests(self) -> None:
        client = jstine.AsyncClient()
        calls: list[tuple[RequestKind, list[tuple[FieldType, bytes]]]] = []
        responses = iter([b"", struct.pack("<Q", 7), b"", b""])

        async def fake_request(kind, fields):
            calls.append((kind, fields))
            return next(responses)

        client._request = fake_request  # type: ignore[method-assign]

        self.assertTrue(await client.setnx("key", "value"))
        self.assertEqual(await client.ttl("key"), 7)
        self.assertTrue(await client.persist("key"))
        self.assertTrue(await client.expire("key", 9))
        self.assertEqual(
            calls,
            [
                (
                    RequestKind.set,
                    [
                        (FieldType.key, b"key"),
                        (FieldType.value, b"value"),
                        (
                            FieldType.set_condition,
                            bytes([SetCondition.nx]),
                        ),
                    ],
                ),
                (RequestKind.ttl, [(FieldType.key, b"key")]),
                (RequestKind.persist, [(FieldType.key, b"key")]),
                (
                    RequestKind.expire,
                    [
                        (FieldType.key, b"key"),
                        (FieldType.seconds, struct.pack("<Q", 9)),
                    ],
                ),
            ],
        )


if __name__ == "__main__":
    unittest.main()
