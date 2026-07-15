from __future__ import annotations

import struct

from assertpy import assert_that

from base.context import Context
from base.wire import RawJFPWire, pack_handshake, pack_request
from jstine._proto import FieldType, Protocol, RequestKind, ResponseKind
from jstine.errors import ErrorCode


def test_ping_roundtrip(ctx: Context) -> None:
    wire = RawJFPWire(ctx).open()
    try:
        wire.handshake()

        kind, fields = wire.request(
            RequestKind.ping,
            [(FieldType.payload, b"Hello wire!")],
        )

        assert_that(kind).is_equal_to(ResponseKind.ok)
        assert_that(fields).contains((FieldType.payload, b"Hello wire!"))
    finally:
        wire.close()


def test_fragmented_request_roundtrip(ctx: Context) -> None:
    wire = RawJFPWire(ctx).open()
    try:
        wire.handshake()

        set_frame = pack_request(
            RequestKind.set,
            [
                (FieldType.key, b"raw:fragmented"),
                (FieldType.value, b"value"),
            ],
        )
        kind, fields = wire.request_chunks(
            header_chunks=(set_frame[:2], set_frame[2:6], set_frame[6:]),
        )

        assert_that(kind).is_equal_to(ResponseKind.ok)
        assert_that(fields).is_empty()

        get_frame = pack_request(
            RequestKind.get,
            [(FieldType.key, b"raw:fragmented")],
        )
        kind, fields = wire.request_chunks(
            header_chunks=(get_frame[:1], get_frame[1:3], get_frame[3:]),
        )
        assert_that(kind).is_equal_to(ResponseKind.ok)
        assert_that(fields).contains((FieldType.payload, b"value"))
    finally:
        wire.close()


def test_multiple_requests_on_same_connection(ctx: Context) -> None:
    wire = RawJFPWire(ctx).open()
    try:
        wire.handshake()

        kind, fields = wire.request(
            RequestKind.set,
            [(FieldType.key, b"raw:reuse"), (FieldType.value, b"first")],
        )
        assert_that(kind).is_equal_to(ResponseKind.ok)
        assert_that(fields).is_empty()

        kind, fields = wire.request(
            RequestKind.get,
            [(FieldType.key, b"raw:reuse")],
        )
        assert_that(kind).is_equal_to(ResponseKind.ok)
        assert_that(fields).contains((FieldType.payload, b"first"))

        kind, fields = wire.request(
            RequestKind.set,
            [(FieldType.key, b"raw:reuse"), (FieldType.value, b"second")],
        )
        assert_that(kind).is_equal_to(ResponseKind.ok)
        assert_that(fields).is_empty()

        kind, fields = wire.request(
            RequestKind.get,
            [(FieldType.key, b"raw:reuse")],
        )
        assert_that(kind).is_equal_to(ResponseKind.ok)
        assert_that(fields).contains((FieldType.payload, b"second"))
    finally:
        wire.close()


def test_delete_missing_returns_not_found_error(ctx: Context) -> None:
    wire = RawJFPWire(ctx).open()
    try:
        wire.handshake()

        kind, fields = wire.request(
            RequestKind.delete,
            [(FieldType.key, b"missing:key")],
        )

        assert_that(kind).is_equal_to(ResponseKind.error)
        assert_that(
            int.from_bytes(
                next(value for field_type,
                     value in fields if field_type == FieldType.error_code),
                "little",
            )
        ).is_equal_to(int(ErrorCode.notFound))
        assert_that(
            next(value for field_type, value in fields if field_type == FieldType.error_message).decode(
                "utf-8"
            )
        ).is_equal_to("Key does not exist")
    finally:
        wire.close()


def test_rejects_invalid_handshake_magic(ctx: Context) -> None:
    wire = RawJFPWire(ctx).open()
    try:
        wire.send(pack_handshake(magic=0))
        wire.wait_for_close()
    finally:
        wire.close()


def test_rejects_invalid_handshake_protocol(ctx: Context) -> None:
    wire = RawJFPWire(ctx).open()
    try:
        wire.send(struct.pack("<II8s", 999, 0xDEADBEEF, bytes(8)))
        wire.wait_for_close()
    finally:
        wire.close()


def test_rejects_unknown_request_kind(ctx: Context) -> None:
    wire = RawJFPWire(ctx).open()
    try:
        wire.handshake()

        wire.send(struct.pack("<II", 4, 999))
        wire.wait_for_close()
    finally:
        wire.close()


def test_rejects_truncated_field_header(ctx: Context) -> None:
    wire = RawJFPWire(ctx).open()
    try:
        wire.handshake()

        wire.send(struct.pack("<II", 5, int(RequestKind.ping)) + b"\x01")
        wire.wait_for_close()
    finally:
        wire.close()


def test_rejects_field_data_out_of_bounds(ctx: Context) -> None:
    wire = RawJFPWire(ctx).open()
    try:
        wire.handshake()

        wire.send(
            struct.pack("<I", 12)
            + struct.pack("<I", int(RequestKind.set))
            + struct.pack("<BI", int(FieldType.key), 8)
            + b"abc"
        )
        wire.wait_for_close()
    finally:
        wire.close()
