from __future__ import annotations

import multiprocessing
import threading
import time
from queue import Empty
from typing import Literal, TypeAlias

import jstine

from .metrics import Metrics
from .recorder import Recorder
from .worker import WorkerInstance


ThreadResult: TypeAlias = Metrics | BaseException
ProcessResult: TypeAlias = tuple[Literal["ok"], Metrics] | tuple[Literal["error"], str]


def run_process(
    host: str,
    port: int,
    duration: float,
    instances: list[WorkerInstance],
    queue: multiprocessing.Queue[ProcessResult],
) -> None:
    try:
        deadline = time.monotonic() + duration
        results = _ThreadResults()
        threads = [
            threading.Thread(
                target=_run_worker,
                args=(host, port, deadline, instance, results),
            )
            for instance in instances
        ]

        for thread in threads:
            thread.start()

        metrics = Metrics()
        for thread in threads:
            thread.join()

        while True:
            try:
                result = results.get_nowait()
            except Empty:
                break

            if isinstance(result, BaseException):
                raise result
            metrics.merge(result)

        queue.put(("ok", metrics))
    except BaseException as exc:
        queue.put(("error", f"{type(exc).__name__}: {exc}"))


class _ThreadResults:
    def __init__(self) -> None:
        import queue

        self._queue: queue.Queue[ThreadResult] = queue.Queue()

    def put(self, item: ThreadResult) -> None:
        self._queue.put(item)

    def get_nowait(self) -> ThreadResult:
        return self._queue.get_nowait()


def _run_worker(
    host: str,
    port: int,
    deadline: float,
    instance: WorkerInstance,
    results: _ThreadResults,
) -> None:
    try:
        metrics = Metrics()
        recorder = Recorder(instance.definition.tag, metrics)

        with jstine.Client(host=host, port=port) as client:
            operation = instance.definition.factory(client, instance.index, recorder)
            while time.monotonic() < deadline:
                operation()

        results.put(metrics)
    except BaseException as exc:
        results.put(exc)
