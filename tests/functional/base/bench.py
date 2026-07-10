import typing
import asyncio
import contextlib
import jstine

from collections.abc import Callable

from . import case
from assertpy import assert_that


def repeat(value: typing.Any) -> typing.Generator[typing.Any, None, None]:
    while True:
        yield value


async def measure(callback: typing.Callable[[], typing.Awaitable[None]]) -> float:
    start = asyncio.get_event_loop().time()
    await callback()
    end = asyncio.get_event_loop().time()
    return end - start


def generator(prefix: str, top: int = 100) -> typing.Generator[str, None, None]:
    i = 0
    while True:
        yield f"{prefix}:{i}"
        i = (i + 1) % top


class _Histogram:
    def __init__(self, name: str):
        self._name = name
        self.values = []

    def observe(self, value: float):
        self.values.append(value)

    def to_str(self) -> str:
        if not self.values:
            return f"> {self._name}: no data"

        s = sum(self.values)
        avg = s / len(self.values)
        min_val = min(self.values)
        max_val = max(self.values)

        return f"> {self._name}: avg {avg:.6f}, min {min_val:.6f}, max {max_val:.6f}"


class _Counter:
    def __init__(self, name: str):
        self._value = 0
        self._name = name

    def observe(self):
        self._value += 1

    def to_str(self, duration: float = None) -> str:
        out = f"> {self._name}: {self._value}"
        if duration is not None:
            out += f" - ({self._value / duration:.2f} ops/s)"
        return out


class Benchmark(case.TestCase):
    def setUp(self):
        super().setUp()

        self.workers: list[asyncio.Task] = []
        self.histograms: dict[str, _Histogram] = {}
        self.counters: dict[str, _Counter] = {}

        self.total_ops: _Counter = self.define_counter("total_ops")
        self.total_sets: _Counter = self.define_counter("total_sets")

        self.set_latencies: _Histogram = self.define_histogram("set_latencies")

    def define_counter(self, name: str) -> _Counter:
        counter = _Counter(name)
        self.counters[name] = counter
        return counter

    def define_histogram(self, name: str) -> _Histogram:
        histogram = _Histogram(name)
        self.histograms[name] = histogram
        return histogram

    def add_worker(
        self,
        worker: Callable[[jstine.AsyncClient], typing.Awaitable[None]]
    ) -> None:
        async def _wrapper() -> None:
            async with jstine.AsyncClient() as c:
                await worker(c)
        self.workers.append(asyncio.create_task(_wrapper()))

    def add_setter(
        self,
        key_generator: typing.Generator[str, None, None],
        value_generator: typing.Generator[str, None, None],
    ) -> None:
        async def _setter(c: jstine.AsyncClient) -> None:
            async def _op():
                ok = await c.set(next(key_generator), next(value_generator))
                assert_that(ok).is_true()

            while True:
                duration = await measure(_op)
                self.total_ops.observe()
                self.total_sets.observe()
                self.set_latencies.observe(duration)

        self.add_worker(_setter)

    async def run_for(self, duration: float) -> None:
        bench_task = asyncio.create_task(self._run())

        await asyncio.sleep(duration)
        bench_task.cancel()

        with contextlib.suppress(asyncio.CancelledError):
            await bench_task

        self._summarize_metrics(duration=duration)

    async def _run(self) -> None:
        benchmarks_tasks = []
        benchmarks_tasks += self.workers

        await asyncio.gather(
            *benchmarks_tasks
        )

    def _summarize_metrics(self, duration: float):
        self._artifact("-- benchmark metrics --\n\n")

        for counter in self.counters.values():
            self._artifact(counter.to_str(duration))

        for histogram in self.histograms.values():
            self._artifact(histogram.to_str())

    def _artifact(self, text: str) -> None:
        filename = 'benchmark.logs'
        self.write_artifact(text, name=filename)
