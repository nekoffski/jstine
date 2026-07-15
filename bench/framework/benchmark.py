from __future__ import annotations

import argparse
import multiprocessing
from collections.abc import Callable

import jstine

from .metrics import Metrics
from .process import ProcessSampler
from .reporting import print_metrics, print_parameters, print_process_metrics
from .runner import ProcessResult, run_process
from .types import BenchmarkInitHook
from .worker import WorkerDefinition, WorkerFactory, WorkerInstance


class Benchmark:
    def __init__(self, name: str):
        self.name = name
        self._workers: list[WorkerDefinition] = []
        self._init: BenchmarkInitHook | None = None

    def worker(
        self,
        tag: str,
        default: int = 1,
    ) -> Callable[[WorkerFactory], WorkerFactory]:
        if default < 0:
            raise ValueError("worker default must be greater than or equal to zero")

        def register(factory: WorkerFactory) -> WorkerFactory:
            if any(worker.tag == tag for worker in self._workers):
                raise ValueError(f"worker tag already exists: {tag}")
            self._workers.append(WorkerDefinition(tag, default, factory))
            return factory

        return register

    def init(
        self,
        hook: BenchmarkInitHook | None = None,
    ) -> BenchmarkInitHook | Callable[[BenchmarkInitHook], BenchmarkInitHook]:
        def register(hook: BenchmarkInitHook) -> BenchmarkInitHook:
            if self._init is not None:
                raise ValueError("benchmark init hook already exists")
            self._init = hook
            return hook

        if hook is None:
            return register

        return register(hook)

    def main(self) -> None:
        args = self._parse_args()
        worker_counts = self._worker_counts(args.workers)
        instances = self._worker_instances(worker_counts)
        if not instances:
            raise SystemExit("no workers configured")
        active_tags = self._active_tags(worker_counts)

        process_count = min(args.processes, len(instances))
        print_parameters(
            host=args.host,
            port=args.port,
            duration=args.duration,
            processes=process_count,
            workers=worker_counts,
            pid=args.pid,
            process_sample_interval=args.process_sample_interval,
        )

        if self._init is not None:
            with jstine.Client(host=args.host, port=args.port) as client:
                self._init(client, active_tags)

        process_sampler = (
            ProcessSampler(args.pid, args.process_sample_interval)
            if args.pid is not None
            else None
        )
        if process_sampler is not None:
            process_sampler.start()

        queue: multiprocessing.Queue[ProcessResult] = multiprocessing.Queue()
        processes = [
            multiprocessing.Process(
                target=run_process,
                args=(
                    args.host,
                    args.port,
                    args.duration,
                    instances[i::process_count],
                    queue,
                ),
            )
            for i in range(process_count)
        ]

        for process in processes:
            process.start()

        metrics = Metrics()
        error: str | None = None
        for _ in processes:
            status, result = queue.get()
            if status == "error":
                error = result
            else:
                metrics.merge(result)

        for process in processes:
            process.join()
            if process.exitcode != 0:
                error = error or f"process exited with code {process.exitcode}"

        process_metrics = (
            process_sampler.stop() if process_sampler is not None else None
        )

        if error is not None:
            raise SystemExit(error)

        print_metrics(metrics, args.duration)
        if process_metrics is not None:
            print()
            print_process_metrics(process_metrics)
        print()

    def _parse_args(self) -> argparse.Namespace:
        parser = argparse.ArgumentParser(prog=self.name)
        parser.add_argument("--host", default="127.0.0.1")
        parser.add_argument("--port", type=int, default=9991)
        parser.add_argument("--duration", type=float, default=15.0)
        parser.add_argument("--processes", type=int, default=1)
        parser.add_argument("--pid", type=int, default=None)
        parser.add_argument("--process-sample-interval", type=float, default=0.25)
        parser.add_argument(
            "--workers",
            default=None,
            help="comma-separated worker counts, for example setter=1,getter=50",
        )
        args = parser.parse_args()

        if args.duration <= 0:
            raise SystemExit("--duration must be greater than zero")
        if args.processes <= 0:
            raise SystemExit("--processes must be greater than zero")
        if args.pid is not None and args.pid <= 0:
            raise SystemExit("--pid must be greater than zero")
        if args.process_sample_interval <= 0:
            raise SystemExit("--process-sample-interval must be greater than zero")

        return args

    def _worker_counts(self, raw: str | None) -> dict[str, int]:
        counts = {worker.tag: worker.default for worker in self._workers}
        if raw is None:
            return counts

        known_tags = set(counts)
        counts = dict.fromkeys(known_tags, 0)
        for part in raw.split(","):
            tag, sep, count_text = part.partition("=")
            if not sep:
                raise SystemExit(f"invalid --workers item: {part}")
            if tag not in known_tags:
                raise SystemExit(f"unknown worker tag: {tag}")

            try:
                count = int(count_text)
            except ValueError as exc:
                raise SystemExit(
                    f"invalid worker count for {tag}: {count_text}"
                ) from exc

            if count < 0:
                raise SystemExit(f"worker count must be non-negative: {tag}")
            counts[tag] = count

        return counts

    def _worker_instances(self, counts: dict[str, int]) -> list[WorkerInstance]:
        instances: list[WorkerInstance] = []
        for definition in self._workers:
            for index in range(counts[definition.tag]):
                instances.append(WorkerInstance(definition, index))
        return instances

    def _active_tags(self, counts: dict[str, int]) -> dict[str, int]:
        return {
            definition.tag: counts[definition.tag]
            for definition in self._workers
            if counts[definition.tag] > 0
        }
