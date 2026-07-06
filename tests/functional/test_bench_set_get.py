import pytest
import asyncio
import time
import asyncio
import contextlib

from base.case import TestCase

from assertpy import assert_that


@pytest.mark.benchmark
class BenchmarkLoadSetGet(TestCase):
    BENCHMARK_DURATION = 20

    def setUp(self):
        super().setUp()
        self.ops = 0

    def tearDown(self):
        self.write_artifact(
            "ops/s: {}".format(self.ops / self.BENCHMARK_DURATION), "benchmark.logs")

    async def set_worker(self, key_generator, value_generator, index):
        async with self.async_client() as c:
            while True:
                k, v = next(key_generator), next(value_generator)
                await c.set(k, v)
                self.ops += 1

    async def test_set_single_client(self):
        def gen(index, prefix):
            i = 0
            while True:
                yield f"{prefix}:{index}:{i}"
                i += 1

        task = asyncio.create_task(
            self.set_worker(gen(0, "key"), gen(0, "value"), 0)
        )

        await asyncio.sleep(self.BENCHMARK_DURATION)
        task.cancel()

        with contextlib.suppress(asyncio.CancelledError):
            await task
