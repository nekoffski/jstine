from __future__ import annotations

import pytest
from assertpy import assert_that

from base.context import Context
from base.harness import AsyncClientHarness
from jstine import AsyncClient

pytestmark = pytest.mark.asyncio


async def test_ping(async_client_harness: AsyncClientHarness) -> None:
    await async_client_harness.assert_ping(b"Hello world!")


async def test_ping_empty_payload(async_client_harness: AsyncClientHarness) -> None:
    await async_client_harness.assert_ping(b"")


async def test_set_get(async_client_harness: AsyncClientHarness) -> None:
    await async_client_harness.assert_set_get(key=b"my:key", value=b"Hello world!")


async def test_set_get_with_coercion(
    async_client_harness: AsyncClientHarness,
) -> None:
    await async_client_harness.assert_set_get(key="my:key:str", value=123)
    assert_that(await async_client_harness.client.get("my:key:str")).is_equal_to(
        b"123"
    )


async def test_exists(async_client_harness: AsyncClientHarness) -> None:
    key = b"my:key"

    await async_client_harness.assert_exists(key=key, expected=False)
    await async_client_harness.client.set(key=key, value=b"Hello world!")
    await async_client_harness.assert_exists(key=key, expected=True)


async def test_missing_get(async_client_harness: AsyncClientHarness) -> None:
    await async_client_harness.assert_missing_get(key=b"non:existent:key")


async def test_delete_existing_key(async_client_harness: AsyncClientHarness) -> None:
    key = b"my:key"
    await async_client_harness.client.set(key=key, value=b"Hello world!")

    await async_client_harness.assert_delete(key=key, expected=True)
    await async_client_harness.assert_exists(key=key, expected=False)


async def test_delete_missing_key(async_client_harness: AsyncClientHarness) -> None:
    await async_client_harness.assert_delete(key=b"missing:key", expected=False)


async def test_del_alias(async_client_harness: AsyncClientHarness) -> None:
    key = b"my:key"
    await async_client_harness.client.set(key=key, value=b"Hello world!")

    await async_client_harness.assert_del(key=key, expected=True)
    await async_client_harness.assert_exists(key=key, expected=False)


async def test_context_manager(ctx: Context) -> None:
    async with AsyncClient(port=ctx.config().server.port) as client:
        assert_that(await client.ping(b"ctx")).is_equal_to(b"ctx")
        assert_that(await client.delete(b"missing:key")).is_false()


async def test_close_is_idempotent(ctx: Context) -> None:
    client = AsyncClient(port=ctx.config().server.port)
    await client.connect()
    await client.close()
    await client.close()


async def test_reconnect_after_close(ctx: Context) -> None:
    client = AsyncClient(port=ctx.config().server.port)
    await client.connect()
    await client.close()
    await client.connect()

    assert_that(await client.ping(b"reconnect")).is_equal_to(b"reconnect")
    await client.close()


async def test_ping_rejects_true_payload(
    async_client_harness: AsyncClientHarness,
) -> None:
    with pytest.raises(TypeError):
        await async_client_harness.client.ping(payload=True)


async def test_ping_rejects_false_payload(
    async_client_harness: AsyncClientHarness,
) -> None:
    with pytest.raises(TypeError):
        await async_client_harness.client.ping(payload=False)
