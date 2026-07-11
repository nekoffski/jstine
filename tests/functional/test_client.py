from __future__ import annotations

import pytest
from assertpy import assert_that

from base.case import TestCase
from base.harness import ClientHarness
from jstine import Client


class TestClientFunctional(TestCase):
    def setUp(self):
        super().setUp()
        self.h = ClientHarness(self)

    def test_ping(self):
        self.h.assert_ping(b"Hello world!")

    def test_ping_empty_payload(self):
        self.h.assert_ping(b"")

    def test_set_get(self):
        self.h.assert_set_get(key=b"my:key", value=b"Hello world!")

    def test_set_get_with_coercion(self):
        self.h.assert_set_get(key="my:key:str", value=123)
        assert_that(self.h.client.get("my:key:str")).is_equal_to(b"123")

    def test_exists(self):
        key = b"my:key"

        self.h.assert_exists(key=key, expected=False)
        self.h.client.set(key=key, value=b"Hello world!")
        self.h.assert_exists(key=key, expected=True)

    def test_missing_get(self):
        self.h.assert_missing_get(key=b"non:existent:key")

    def test_delete_existing_key(self):
        key = b"my:key"
        self.h.client.set(key=key, value=b"Hello world!")

        self.h.assert_delete(key=key, expected=True)
        self.h.assert_exists(key=key, expected=False)

    def test_delete_missing_key(self):
        self.h.assert_delete(key=b"missing:key", expected=False)

    def test_del_alias(self):
        key = b"my:key"
        self.h.client.set(key=key, value=b"Hello world!")

        self.h.assert_del(key=key, expected=True)
        self.h.assert_exists(key=key, expected=False)

    def test_context_manager(self):
        with Client(port=self.config().server.port) as client:
            assert_that(client.ping(b"ctx")).is_equal_to(b"ctx")
            assert_that(client.delete(b"missing:key")).is_false()

    def test_close_is_idempotent(self):
        client = Client(port=self.config().server.port)
        client.connect()
        client.close()
        client.close()

    def test_reconnect_after_close(self):
        client = Client(port=self.config().server.port)
        client.connect()
        client.close()
        client.connect()

        assert_that(client.ping(b"reconnect")).is_equal_to(b"reconnect")
        client.close()

    def test_ping_rejects_true_payload(self):
        with pytest.raises(TypeError):
            self.h.client.ping(payload=True)

    def test_ping_rejects_false_payload(self):
        with pytest.raises(TypeError):
            self.h.client.ping(payload=False)
