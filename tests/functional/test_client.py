from __future__ import annotations

import pytest
from assertpy import assert_that

from base.context import Context
from base.harness import ClientHarness
from jstine import Client


def test_ping(client_harness: ClientHarness) -> None:
    client_harness.assert_ping(b"Hello world!")


def test_ping_empty_payload(client_harness: ClientHarness) -> None:
    client_harness.assert_ping(b"")


def test_set_get(client_harness: ClientHarness) -> None:
    client_harness.assert_set_get(key=b"my:key", value=b"Hello world!")


def test_set_get_with_coercion(client_harness: ClientHarness) -> None:
    client_harness.assert_set_get(key="my:key:str", value=123)
    assert_that(client_harness.client.get("my:key:str")).is_equal_to(b"123")


def test_exists(client_harness: ClientHarness) -> None:
    key = b"my:key"

    client_harness.assert_exists(key=key, expected=False)
    client_harness.client.set(key=key, value=b"Hello world!")
    client_harness.assert_exists(key=key, expected=True)


def test_missing_get(client_harness: ClientHarness) -> None:
    client_harness.assert_missing_get(key=b"non:existent:key")


def test_delete_existing_key(client_harness: ClientHarness) -> None:
    key = b"my:key"
    client_harness.client.set(key=key, value=b"Hello world!")

    client_harness.assert_delete(key=key, expected=True)
    client_harness.assert_exists(key=key, expected=False)


def test_delete_missing_key(client_harness: ClientHarness) -> None:
    client_harness.assert_delete(key=b"missing:key", expected=False)


def test_del_alias(client_harness: ClientHarness) -> None:
    key = b"my:key"
    client_harness.client.set(key=key, value=b"Hello world!")

    client_harness.assert_del(key=key, expected=True)
    client_harness.assert_exists(key=key, expected=False)


def test_context_manager(ctx: Context) -> None:
    with Client(port=ctx.config().server.port) as client:
        assert_that(client.ping(b"ctx")).is_equal_to(b"ctx")
        assert_that(client.delete(b"missing:key")).is_false()


def test_close_is_idempotent(ctx: Context) -> None:
    client = Client(port=ctx.config().server.port)
    client.connect()
    client.close()
    client.close()


def test_reconnect_after_close(ctx: Context) -> None:
    client = Client(port=ctx.config().server.port)
    client.connect()
    client.close()
    client.connect()

    assert_that(client.ping(b"reconnect")).is_equal_to(b"reconnect")
    client.close()


def test_ping_rejects_true_payload(client_harness: ClientHarness) -> None:
    with pytest.raises(TypeError):
        client_harness.client.ping(payload=True)


def test_ping_rejects_false_payload(client_harness: ClientHarness) -> None:
    with pytest.raises(TypeError):
        client_harness.client.ping(payload=False)
