from __future__ import annotations

from contextlib import suppress

from jstine import AsyncClient
from jstine import Client
from jstine._common import coerce_bytes
from assertpy import assert_that


class ClientHarness:
    def __init__(
        self,
        testcase,
        client: Client | None = None,
        add_cleanup: bool = True,
    ) -> None:
        self.client = client or testcase.client()
        self.client.connect()
        if add_cleanup:
            testcase.addCleanup(self.close)

    def close(self) -> None:
        self.client.close()

    def assert_ping(
        self,
        payload: bytes | bytearray | memoryview | str | int | float,
    ) -> None:
        pong = self.client.ping(payload=payload)
        assert_that(pong).is_equal_to(coerce_bytes(payload))

    def assert_set_get(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
        value: bytes | bytearray | memoryview | str | int | float,
    ) -> None:
        resp = self.client.set(key=key, value=value)
        assert_that(resp).is_true()

        got = self.client.get(key=key)
        assert_that(got).is_equal_to(coerce_bytes(value))

    def assert_exists(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
        expected: bool,
    ) -> None:
        exists = self.client.exists(key=key)
        assert_that(exists).is_equal_to(expected)

    def assert_overwrite(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
        value: bytes | bytearray | memoryview | str | int | float,
        updated_value: bytes | bytearray | memoryview | str | int | float,
    ) -> None:
        resp = self.client.set(key=key, value=value)
        assert_that(resp).is_true()

        resp = self.client.get(key=key)
        assert_that(resp).is_equal_to(coerce_bytes(value))

        resp = self.client.set(key=key, value=updated_value)
        assert_that(resp).is_true()

        got = self.client.get(key=key)
        assert_that(got).is_equal_to(coerce_bytes(updated_value))

    def assert_missing_get(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
    ) -> None:
        got = self.client.get(key=key)
        assert_that(got).is_none()

    def assert_delete(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
        expected: bool,
    ) -> None:
        deleted = self.client.delete(key=key)
        assert_that(deleted).is_equal_to(expected)

    def assert_del(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
        expected: bool,
    ) -> None:
        deleted = self.client.del_(key=key)
        assert_that(deleted).is_equal_to(expected)


class AsyncClientHarness:
    def __init__(self, client: AsyncClient) -> None:
        self.client = client

    @classmethod
    async def create(
        cls,
        testcase,
        client: AsyncClient | None = None,
        add_cleanup: bool = True,
    ) -> "AsyncClientHarness":
        instance = cls(client or testcase.async_client())
        await instance.client.connect()
        if add_cleanup:
            testcase.addAsyncCleanup(instance.close)
        return instance

    async def close(self) -> None:
        with suppress(Exception):
            await self.client.close()

    async def assert_ping(
        self,
        payload: bytes | bytearray | memoryview | str | int | float,
    ) -> None:
        pong = await self.client.ping(payload=payload)
        assert_that(pong).is_equal_to(coerce_bytes(payload))

    async def assert_set_get(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
        value: bytes | bytearray | memoryview | str | int | float,
    ) -> None:
        resp = await self.client.set(key=key, value=value)
        assert_that(resp).is_true()

        got = await self.client.get(key=key)
        assert_that(got).is_equal_to(coerce_bytes(value))

    async def assert_exists(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
        expected: bool,
    ) -> None:
        exists = await self.client.exists(key=key)
        assert_that(exists).is_equal_to(expected)

    async def assert_missing_get(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
    ) -> None:
        got = await self.client.get(key=key)
        assert_that(got).is_none()

    async def assert_delete(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
        expected: bool,
    ) -> None:
        deleted = await self.client.delete(key=key)
        assert_that(deleted).is_equal_to(expected)

    async def assert_del(
        self,
        key: bytes | bytearray | memoryview | str | int | float,
        expected: bool,
    ) -> None:
        deleted = await self.client.del_(key=key)
        assert_that(deleted).is_equal_to(expected)
