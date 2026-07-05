import pytest
import asyncio

from base.case import TestCase

from assertpy import assert_that


@pytest.mark.benchmark
class BenchmarkBasic(TestCase):
    async def test_multiple_pings(self):
        clients = 10
        N = 1000

        tasks = []

        async def ping_task():
            async with self.async_client() as c:
                for _ in range(N):
                    resp = await c.ping(payload=b"Hello world!")
                    assert_that(resp).is_equal_to(b"Hello world!")

        for _ in range(clients):
            tasks.append(asyncio.create_task(ping_task()))

        await asyncio.gather(*tasks)
