from __future__ import annotations

import pytest
from assertpy import assert_that

from base.case import TestCase
from base.harness import AsyncClientHarness
from jstine import AsyncClient


class TestAsyncClientFunctional(TestCase):
    async def test_ping(self):
        h = await AsyncClientHarness.create(self)
        await h.assert_ping(b"Hello world!")

    async def test_ping_empty_payload(self):
        h = await AsyncClientHarness.create(self)
        await h.assert_ping(b"")

    async def test_set_get(self):
        h = await AsyncClientHarness.create(self)
        await h.assert_set_get(key=b"my:key", value=b"Hello world!")

    async def test_set_get_with_coercion(self):
        h = await AsyncClientHarness.create(self)
        await h.assert_set_get(key="my:key:str", value=123)
        assert_that(await h.client.get("my:key:str")).is_equal_to(b"123")

    async def test_exists(self):
        h = await AsyncClientHarness.create(self)
        key = b"my:key"

        await h.assert_exists(key=key, expected=False)
        await h.client.set(key=key, value=b"Hello world!")
        await h.assert_exists(key=key, expected=True)

    async def test_missing_get(self):
        h = await AsyncClientHarness.create(self)
        await h.assert_missing_get(key=b"non:existent:key")

    async def test_delete_existing_key(self):
        h = await AsyncClientHarness.create(self)
        key = b"my:key"
        await h.client.set(key=key, value=b"Hello world!")

        await h.assert_delete(key=key, expected=True)
        await h.assert_exists(key=key, expected=False)

    async def test_delete_missing_key(self):
        h = await AsyncClientHarness.create(self)
        await h.assert_delete(key=b"missing:key", expected=False)

    async def test_del_alias(self):
        h = await AsyncClientHarness.create(self)
        key = b"my:key"
        await h.client.set(key=key, value=b"Hello world!")

        await h.assert_del(key=key, expected=True)
        await h.assert_exists(key=key, expected=False)

    async def test_context_manager(self):
        async with AsyncClient(port=self.config().server.port) as client:
            assert_that(await client.ping(b"ctx")).is_equal_to(b"ctx")
            assert_that(await client.delete(b"missing:key")).is_false()

    async def test_close_is_idempotent(self):
        client = AsyncClient(port=self.config().server.port)
        await client.connect()
        await client.close()
        await client.close()

    async def test_reconnect_after_close(self):
        client = AsyncClient(port=self.config().server.port)
        await client.connect()
        await client.close()
        await client.connect()

        assert_that(await client.ping(b"reconnect")).is_equal_to(b"reconnect")
        await client.close()

    async def test_ping_rejects_true_payload(self):
        h = await AsyncClientHarness.create(self)
        with pytest.raises(TypeError):
            await h.client.ping(payload=True)

    async def test_ping_rejects_false_payload(self):
        h = await AsyncClientHarness.create(self)
        with pytest.raises(TypeError):
            await h.client.ping(payload=False)
