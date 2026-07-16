from __future__ import annotations

import argparse
import contextlib
import multiprocessing
from collections.abc import Callable
from pathlib import Path

import jstine

from .metrics import Metrics
from .process import ProcessSampler
from .reporting import print_metrics, print_process_metrics
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
        worker_counts, run_tags = self._parse_tags(args.tags)
        instances = self._worker_instances(worker_counts)
        if not instances:
            raise SystemExit("no workers configured")
        active_tags = self._active_tags(worker_counts, run_tags)

        process_count = min(args.processes, len(instances))
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
                    active_tags,
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

        print_metrics(metrics, args.duration, args.name)
        if process_metrics is not None:
            if args.process_metrics_output == "stdout":
                print()
                print_process_metrics(process_metrics)
            elif args.process_metrics_output == "file":
                if args.process_metrics_file is None:
                    raise SystemExit(
                        "--process-metrics-file is required when "
                        "--process-metrics-output=file"
                    )
                path = Path(args.process_metrics_file)
                path.parent.mkdir(parents=True, exist_ok=True)
                with path.open("w", encoding="utf-8") as stream:
                    with contextlib.redirect_stdout(stream):
                        print_process_metrics(process_metrics)
        print()

    def _parse_args(self) -> argparse.Namespace:
        parser = argparse.ArgumentParser(prog=self.name)
        parser.add_argument("--host", default="127.0.0.1")
        parser.add_argument("--port", type=int, default=9991)
        parser.add_argument("--duration", type=float, default=15.0)
        parser.add_argument("--processes", type=int, default=1)
        parser.add_argument("--name", default=self.name)
        parser.add_argument("--pid", type=int, default=None)
        parser.add_argument("--process-sample-interval", type=float, default=0.25)
        parser.add_argument(
            "--process-metrics-output",
            choices=("stdout", "file"),
            default="stdout",
        )
        parser.add_argument("--process-metrics-file", default=None)
        parser.add_argument(
            "--tags",
            default=None,
            help="comma-separated tag counts, for example setter=1,getter=50",
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

    def _parse_tags(self, raw: str | None) -> tuple[dict[str, int], dict[str, int]]:
        default_counts = {worker.tag: worker.default for worker in self._workers}
        if raw is None:
            return default_counts, {}

        known_tags = set(default_counts)
        parsed_tags: dict[str, int] = {}
        for part in raw.split(","):
            tag, sep, count_text = part.partition("=")
            if not sep:
                raise SystemExit(f"invalid --tags item: {part}")

            try:
                count = int(count_text)
            except ValueError as exc:
                raise SystemExit(
                    f"invalid tag count for {tag}: {count_text}"
                ) from exc

            if count < 0:
                raise SystemExit(f"tag count must be non-negative: {tag}")
            parsed_tags[tag] = count

        worker_tags = known_tags & set(parsed_tags)
        counts = (
            dict.fromkeys(known_tags, 0)
            if worker_tags
            else default_counts.copy()
        )
        for tag in worker_tags:
            counts[tag] = parsed_tags[tag]

        return counts, parsed_tags

    def _worker_instances(self, counts: dict[str, int]) -> list[WorkerInstance]:
        instances: list[WorkerInstance] = []
        for definition in self._workers:
            for index in range(counts[definition.tag]):
                instances.append(WorkerInstance(definition, index))
        return instances

    def _active_tags(
        self,
        counts: dict[str, int],
        tags: dict[str, int],
    ) -> dict[str, int]:
        return tags | {
            definition.tag: counts[definition.tag]
            for definition in self._workers
            if counts[definition.tag] > 0
        }
