from __future__ import annotations

import asyncio
import threading

import pytest
from assertpy import assert_that

from base.context import Context
from base.harness import AsyncClientHarness, ClientHarness


def test_parallel_writes_to_distinct_keys(ctx: Context) -> None:
    keys = [f"multi:sync:{i}".encode("ascii") for i in range(8)]
    start = threading.Barrier(len(keys))
    errors: list[BaseException] = []
    errors_lock = threading.Lock()

    def worker(index: int, key: bytes) -> None:
        h = ClientHarness(ctx)
        try:
            start.wait()
            h.assert_set_get(key=key, value=f"value:{index}".encode("ascii"))
            h.assert_exists(key=key, expected=True)
        except BaseException as exc:
            with errors_lock:
                errors.append(exc)
            raise
        finally:
            h.close()

    threads = [
        threading.Thread(target=worker, args=(index, key), daemon=True)
        for index, key in enumerate(keys)
    ]

    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()

    assert_that(errors).is_empty()

    probe = ClientHarness(ctx)
    try:
        for index, key in enumerate(keys):
            assert_that(probe.client.get(key=key)).is_equal_to(
                f"value:{index}".encode("ascii")
            )
    finally:
        probe.close()


def test_concurrent_updates_are_visible_across_clients(ctx: Context) -> None:
    key = b"multi:shared"
    writer_one_written = threading.Event()
    writer_two_written = threading.Event()
    values: list[bytes] = []
    errors: list[BaseException] = []
    errors_lock = threading.Lock()

    def writer_one() -> None:
        h = ClientHarness(ctx)
        try:
            h.assert_delete(key=key, expected=False)
            assert_that(h.client.get(key=key)).is_none()
            assert_that(h.client.set(key=key, value=b"writer-one")).is_true()
            assert_that(h.client.get(key=key)).is_equal_to(b"writer-one")
            writer_one_written.set()
            assert_that(writer_two_written.wait(timeout=2.0)).is_true()
            assert_that(h.client.get(key=key)).is_equal_to(b"writer-two")
            values.append(b"writer-one")
        except BaseException as exc:
            with errors_lock:
                errors.append(exc)
            raise
        finally:
            h.close()

    def writer_two() -> None:
        h = ClientHarness(ctx)
        try:
            assert_that(writer_one_written.wait(timeout=2.0)).is_true()
            assert_that(h.client.get(key=key)).is_equal_to(b"writer-one")
            assert_that(h.client.set(key=key, value=b"writer-two")).is_true()
            assert_that(h.client.get(key=key)).is_equal_to(b"writer-two")
            writer_two_written.set()
            assert_that(h.client.get(key=key)).is_equal_to(b"writer-two")
            values.append(b"writer-two")
        except BaseException as exc:
            with errors_lock:
                errors.append(exc)
            raise
        finally:
            h.close()

    t1 = threading.Thread(target=writer_one, daemon=True)
    t2 = threading.Thread(target=writer_two, daemon=True)

    t1.start()
    t2.start()
    t1.join()
    t2.join()

    assert_that(errors).is_empty()
    assert_that(values).contains(b"writer-one", b"writer-two")
    probe = ClientHarness(ctx)
    try:
        assert_that(probe.client.get(key=key)).is_equal_to(b"writer-two")
    finally:
        probe.close()


def test_delete_and_get_across_independent_clients(ctx: Context) -> None:
    key = b"multi:delete"
    writer = ClientHarness(ctx)
    reader = ClientHarness(ctx)
    deleter = ClientHarness(ctx)

    try:
        writer.client.set(key=key, value=b"value")

        deleted = deleter.client.delete(key=key)
        assert_that(deleted).is_true()

        assert_that(reader.client.get(key=key)).is_none()
        assert_that(reader.client.exists(key=key)).is_false()
        assert_that(writer.client.delete(key=key)).is_false()
    finally:
        writer.close()
        reader.close()
        deleter.close()


@pytest.mark.asyncio
async def test_parallel_async_writes_to_distinct_keys(ctx: Context) -> None:
    harnesses = [await AsyncClientHarness.create(ctx) for _ in range(6)]
    keys = [f"multi:async:{i}".encode("ascii") for i in range(len(harnesses))]

    try:
        await asyncio.gather(
            *(
                h.assert_set_get(key=key, value=f"value:{index}".encode("ascii"))
                for index, (h, key) in enumerate(zip(harnesses, keys, strict=True))
            )
        )

        await asyncio.gather(
            *(
                h.assert_exists(key=key, expected=True)
                for h, key in zip(harnesses, keys, strict=True)
            )
        )

        probe = await AsyncClientHarness.create(ctx)
        try:
            for index, key in enumerate(keys):
                assert_that(await probe.client.get(key=key)).is_equal_to(
                    f"value:{index}".encode("ascii")
                )
        finally:
            await probe.close()
    finally:
        await asyncio.gather(*(h.close() for h in harnesses))


@pytest.mark.asyncio
async def test_shared_key_is_visible_across_async_clients(ctx: Context) -> None:
    writer_a = await AsyncClientHarness.create(ctx)
    writer_b = await AsyncClientHarness.create(ctx)
    reader = await AsyncClientHarness.create(ctx)
    key = b"multi:async:shared"

    try:
        await writer_a.client.delete(key=key)
        assert_that(await reader.client.get(key=key)).is_none()

        assert_that(await writer_a.client.set(key=key, value=b"alpha")).is_true()
        assert_that(await reader.client.get(key=key)).is_equal_to(b"alpha")

        assert_that(await writer_b.client.get(key=key)).is_equal_to(b"alpha")
        assert_that(await writer_b.client.set(key=key, value=b"beta")).is_true()
        assert_that(await reader.client.get(key=key)).is_equal_to(b"beta")
        assert_that(await writer_a.client.exists(key=key)).is_true()

        assert_that(await writer_a.client.delete(key=key)).is_true()
        assert_that(await reader.client.get(key=key)).is_none()
        assert_that(await writer_b.client.delete(key=key)).is_false()
    finally:
        await asyncio.gather(writer_a.close(), writer_b.close(), reader.close())
