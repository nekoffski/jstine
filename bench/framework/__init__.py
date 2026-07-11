from __future__ import annotations

import argparse
import contextlib
import multiprocessing
import threading
import time
from collections.abc import Callable, Iterator
from dataclasses import dataclass, field
from pathlib import Path
from queue import Empty
from typing import Literal, TypeAlias

import jstine
import psutil


Operation: TypeAlias = Literal["set", "get"]
BenchmarkOperation: TypeAlias = Callable[[], None]
Client = jstine.Client
BenchmarkInitHook: TypeAlias = Callable[[Client, dict[str, int]], None]


@dataclass(slots=True)
class Histogram:
    count: int = 0
    total: float = 0.0
    minimum: float | None = None
    maximum: float | None = None

    def observe(self, value: float) -> None:
        self.count += 1
        self.total += value
        self.minimum = value if self.minimum is None else min(
            self.minimum, value)
        self.maximum = value if self.maximum is None else max(
            self.maximum, value)

    def merge(self, other: "Histogram") -> None:
        self.count += other.count
        self.total += other.total
        if other.minimum is not None:
            self.minimum = (
                other.minimum
                if self.minimum is None
                else min(self.minimum, other.minimum)
            )
        if other.maximum is not None:
            self.maximum = (
                other.maximum
                if self.maximum is None
                else max(self.maximum, other.maximum)
            )


@dataclass(slots=True)
class Metrics:
    total_sets: int = 0
    total_gets: int = 0
    errors: int = 0
    set_latencies: Histogram = field(default_factory=Histogram)
    get_latencies: Histogram = field(default_factory=Histogram)
    worker_ops: dict[str, int] = field(default_factory=dict)

    @property
    def total_ops(self) -> int:
        return self.total_sets + self.total_gets

    def observe(self, tag: str, operation: Operation, latency: float) -> None:
        if operation == "set":
            self.total_sets += 1
            self.set_latencies.observe(latency)
        else:
            self.total_gets += 1
            self.get_latencies.observe(latency)

        self.worker_ops[tag] = self.worker_ops.get(tag, 0) + 1

    def merge(self, other: "Metrics") -> None:
        self.total_sets += other.total_sets
        self.total_gets += other.total_gets
        self.errors += other.errors
        self.set_latencies.merge(other.set_latencies)
        self.get_latencies.merge(other.get_latencies)

        for tag, count in other.worker_ops.items():
            self.worker_ops[tag] = self.worker_ops.get(tag, 0) + count


@dataclass(slots=True)
class ProcessSnapshot:
    timestamp: float
    cpu_times: dict[str, float]
    memory_kib: dict[str, int]
    context_switches: dict[str, int]
    io: dict[str, int]
    page_faults: dict[str, int]
    num_threads: int | None
    num_fds: int | None

    @property
    def cpu_time_s(self) -> float | None:
        if not self.cpu_times:
            return None
        return sum(self.cpu_times.values())


@dataclass(slots=True)
class ProcessMetrics:
    pid: int
    sample_interval: float
    samples: int = 0
    cpu_percent: float | None = None
    cpu_times: dict[str, float] = field(default_factory=dict)
    memory_kib: dict[str, int] = field(default_factory=dict)
    peak_rss_kib: int | None = None
    context_switches: dict[str, int] = field(default_factory=dict)
    io: dict[str, int] = field(default_factory=dict)
    page_faults: dict[str, int] = field(default_factory=dict)
    num_threads: int | None = None
    num_fds: int | None = None
    runtime_s: float | None = None


class ProcessSampler:
    def __init__(self, pid: int, sample_interval: float):
        self.pid = pid
        self.sample_interval = sample_interval
        self._samples: list[ProcessSnapshot] = []
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)

    def start(self) -> None:
        self._samples.append(_read_process_snapshot(self.pid))
        self._thread.start()

    def stop(self) -> ProcessMetrics:
        self._stop.set()
        self._thread.join()
        return self.metrics()

    def metrics(self) -> ProcessMetrics:
        if not self._samples:
            return ProcessMetrics(pid=self.pid, sample_interval=self.sample_interval)

        first = self._samples[0]
        latest = self._samples[-1]
        runtime = max(0.0, latest.timestamp - first.timestamp)

        cpu_percent = None
        if first.cpu_time_s is not None and latest.cpu_time_s is not None and runtime > 0:
            cpu_percent = (latest.cpu_time_s -
                           first.cpu_time_s) / runtime * 100.0

        return ProcessMetrics(
            pid=self.pid,
            sample_interval=self.sample_interval,
            samples=len(self._samples),
            cpu_percent=cpu_percent,
            cpu_times=_dict_delta(first.cpu_times, latest.cpu_times),
            memory_kib=latest.memory_kib,
            peak_rss_kib=max(
                sample.memory_kib.get("rss", 0) for sample in self._samples
            ),
            context_switches=_dict_delta(
                first.context_switches, latest.context_switches
            ),
            io=_dict_delta(first.io, latest.io),
            page_faults=_dict_delta(first.page_faults, latest.page_faults),
            num_threads=latest.num_threads,
            num_fds=latest.num_fds,
            runtime_s=runtime,
        )

    def _run(self) -> None:
        while not self._stop.wait(self.sample_interval):
            try:
                self._samples.append(_read_process_snapshot(self.pid))
            except OSError:
                break


class Recorder:
    def __init__(self, tag: str, metrics: Metrics):
        self._tag = tag
        self._metrics = metrics

    @contextlib.contextmanager
    def set(self) -> Iterator[None]:
        start = time.monotonic()
        try:
            yield
        except Exception:
            self._metrics.errors += 1
            raise
        finally:
            self._metrics.observe(self._tag, "set", time.monotonic() - start)

    @contextlib.contextmanager
    def get(self) -> Iterator[None]:
        start = time.monotonic()
        try:
            yield
        except Exception:
            self._metrics.errors += 1
            raise
        finally:
            self._metrics.observe(self._tag, "get", time.monotonic() - start)


WorkerFactory: TypeAlias = Callable[[
    Client, int, Recorder], BenchmarkOperation]
ThreadResult: TypeAlias = Metrics | BaseException
ProcessResult: TypeAlias = tuple[Literal["ok"],
                                 Metrics] | tuple[Literal["error"], str]


@dataclass(frozen=True, slots=True)
class WorkerDefinition:
    tag: str
    default: int
    factory: WorkerFactory


@dataclass(frozen=True, slots=True)
class WorkerInstance:
    definition: WorkerDefinition
    index: int


class Benchmark:
    def __init__(self, name: str):
        self.name = name
        self._workers: list[WorkerDefinition] = []
        self._init: BenchmarkInitHook | None = None

    def worker(self, tag: str, default: int = 1) -> Callable[[WorkerFactory], WorkerFactory]:
        if default < 0:
            raise ValueError(
                "worker default must be greater than or equal to zero")

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
        _print_parameters(
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
                target=_run_process,
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

        process_metrics = process_sampler.stop() if process_sampler is not None else None

        if error is not None:
            raise SystemExit(error)

        _print_metrics(metrics, args.duration)
        if process_metrics is not None:
            print()
            _print_process_metrics(process_metrics)
        print()

    def _parse_args(self) -> argparse.Namespace:
        parser = argparse.ArgumentParser(prog=self.name)
        parser.add_argument("--host", default="127.0.0.1")
        parser.add_argument("--port", type=int, default=9991)
        parser.add_argument("--duration", type=float, default=15.0)
        parser.add_argument("--processes", type=int, default=1)
        parser.add_argument("--pid", type=int, default=None)
        parser.add_argument("--process-sample-interval",
                            type=float, default=0.25)
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
            raise SystemExit(
                "--process-sample-interval must be greater than zero")

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
                    f"invalid worker count for {tag}: {count_text}") from exc

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


def _run_process(
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
            operation = instance.definition.factory(
                client, instance.index, recorder)
            while time.monotonic() < deadline:
                operation()

        results.put(metrics)
    except BaseException as exc:
        results.put(exc)


def _print_metrics(metrics: Metrics, duration: float) -> None:
    _print_section("Benchmark")
    _print_rows(
        [
            ("total ops", _format_counter(metrics.total_ops, duration)),
            ("set ops", _format_counter(metrics.total_sets, duration)),
            ("get ops", _format_counter(metrics.total_gets, duration)),
            ("errors", str(metrics.errors)),
        ]
    )

    if metrics.worker_ops:
        print()
        _print_subsection("Workers")
        _print_rows(
            [
                (tag, _format_counter(metrics.worker_ops[tag], duration))
                for tag in sorted(metrics.worker_ops)
            ]
        )

    print()
    _print_subsection("Latency")
    _print_rows(
        [
            ("set", _format_histogram(metrics.set_latencies)),
            ("get", _format_histogram(metrics.get_latencies)),
        ]
    )


def _print_parameters(
    host: str,
    port: int,
    duration: float,
    processes: int,
    workers: dict[str, int],
    pid: int | None,
    process_sample_interval: float,
) -> None:
    worker_text = ", ".join(f"{tag}={count}" for tag,
                            count in sorted(workers.items()))
    rows = [
        ("target", f"{host}:{port}"),
        ("duration", f"{duration:.2f}s"),
        ("processes", str(processes)),
        ("workers", worker_text),
    ]
    if pid is not None:
        rows.extend(
            [
                ("server pid", str(pid)),
                ("sample interval", f"{process_sample_interval:.3f}s"),
            ]
        )

    _print_section("Run")
    _print_rows(rows)
    print()


def _print_process_metrics(metrics: ProcessMetrics) -> None:
    _print_section("Server Process")
    rows = [
        ("pid", str(metrics.pid)),
        ("samples", f"{metrics.samples} @ {metrics.sample_interval:.3f}s"),
    ]
    if metrics.runtime_s is not None:
        rows.append(("runtime", f"{metrics.runtime_s:.3f}s"))
    if metrics.cpu_percent is not None:
        rows.append(("avg cpu", f"{metrics.cpu_percent:.2f}% of one core"))
    if metrics.num_threads is not None:
        rows.append(("threads", str(metrics.num_threads)))
    if metrics.num_fds is not None:
        rows.append(("fds", str(metrics.num_fds)))
    _print_rows(rows)

    if metrics.cpu_times:
        print()
        _print_subsection("CPU Time")
        _print_rows(
            [
                (_humanize_name(name), f"{value:.3f}s")
                for name, value in sorted(metrics.cpu_times.items())
            ]
        )

    memory_rows = [
        (_humanize_name(name), _format_kib(value))
        for name, value in sorted(metrics.memory_kib.items())
    ]
    memory_rows.append(("peak rss", _format_kib(metrics.peak_rss_kib)))
    print()
    _print_subsection("Memory")
    _print_rows(memory_rows)

    if metrics.context_switches:
        print()
        _print_subsection("Context Switches")
        _print_rows(
            [
                (_humanize_name(name), f"{value:,}")
                for name, value in sorted(metrics.context_switches.items())
            ]
        )

    if metrics.page_faults:
        print()
        _print_subsection("Page Faults")
        _print_rows(
            [
                (_humanize_name(name), f"{value:,}")
                for name, value in sorted(metrics.page_faults.items())
            ]
        )

    if metrics.io:
        print()
        _print_subsection("I/O")
        _print_rows(
            [
                (_humanize_name(name), _format_io(name, value))
                for name, value in sorted(metrics.io.items())
            ]
        )


def _read_process_snapshot(pid: int) -> ProcessSnapshot:
    process = psutil.Process(pid)

    with process.oneshot():
        return ProcessSnapshot(
            timestamp=time.monotonic(),
            cpu_times=_as_float_dict(process.cpu_times()),
            memory_kib=_memory_kib(process),
            context_switches=_as_int_dict(process.num_ctx_switches()),
            io=_process_io(process),
            page_faults=_page_faults(pid, process),
            num_threads=_optional_int(process.num_threads),
            num_fds=_num_fds(process),
        )


def _memory_kib(process: psutil.Process) -> dict[str, int]:
    memory = _memory_fields_kib(process.memory_info())

    try:
        full_memory = _memory_fields_kib(process.memory_full_info())
    except (psutil.AccessDenied, psutil.NoSuchProcess):
        return memory

    for name in ("uss", "pss", "swap"):
        if name in full_memory:
            memory[name] = full_memory[name]
    return memory


def _process_io(process: psutil.Process) -> dict[str, int]:
    try:
        return _as_int_dict(process.io_counters())
    except (AttributeError, psutil.AccessDenied, psutil.NoSuchProcess):
        return {}


def _page_faults(pid: int, process: psutil.Process) -> dict[str, int]:
    faults = {
        name: value
        for name, value in _as_int_dict(process.memory_info()).items()
        if name in ("pfaults", "pageins")
    }
    faults.update(_linux_page_faults(pid))
    return faults


def _linux_page_faults(pid: int) -> dict[str, int]:
    stat_path = Path(f"/proc/{pid}/stat")
    if not stat_path.exists():
        return {}

    stat_text = stat_path.read_text(encoding="utf-8").strip()
    end_comm = stat_text.rfind(") ")
    if end_comm == -1:
        return {}

    fields = stat_text[end_comm + 2:].split()
    if len(fields) < 10:
        return {}

    return {
        "minor": int(fields[7]),
        "major": int(fields[9]),
    }


def _num_fds(process: psutil.Process) -> int | None:
    try:
        return process.num_fds()
    except (AttributeError, psutil.AccessDenied, psutil.NoSuchProcess):
        return None


def _optional_int(callback: Callable[[], int]) -> int | None:
    try:
        return callback()
    except (psutil.AccessDenied, psutil.NoSuchProcess):
        return None


def _dict_delta(
    first: dict[str, int | float],
    latest: dict[str, int | float],
) -> dict[str, int | float]:
    return {
        key: latest[key] - first.get(key, 0)
        for key in latest
    }


def _as_float_dict(value: object) -> dict[str, float]:
    if not hasattr(value, "_fields"):
        return {}
    return {
        field: float(getattr(value, field))
        for field in value._fields
    }


def _as_int_dict(value: object) -> dict[str, int]:
    if not hasattr(value, "_fields"):
        return {}
    return {
        field: int(getattr(value, field))
        for field in value._fields
    }


def _memory_fields_kib(value: object) -> dict[str, int]:
    return {
        name: bytes_value // 1024
        for name, bytes_value in _as_int_dict(value).items()
        if name not in ("pfaults", "pageins")
    }
    return {
        name: bytes_value // 1024
        for name, bytes_value in _as_int_dict(value).items()
    }


def _print_section(title: str) -> None:
    print()
    print(title)
    print("-" * len(title))


def _print_subsection(title: str) -> None:
    print(f"- {title}")


def _print_rows(rows: list[tuple[str, str]]) -> None:
    if not rows:
        return

    width = max(len(label) for label, _ in rows)
    for label, value in rows:
        print(f"  {label:<{width}}  {value}")


def _format_counter(value: int, duration: float | None) -> str:
    text = f"{value:,}"
    if duration is not None:
        text += f" ({value / duration:,.2f}/s)"
    return text


def _format_histogram(histogram: Histogram) -> str:
    if histogram.count == 0:
        return "no data"

    average = histogram.total / histogram.count
    return (
        f"avg {_format_seconds(average)}, "
        f"min {_format_seconds(histogram.minimum)}, "
        f"max {_format_seconds(histogram.maximum)}"
    )


def _format_kib(value: int | None) -> str:
    if value is None:
        return "n/a"
    return f"{value} KiB ({value / 1024.0:.2f} MiB)"


def _format_seconds(value: float | None) -> str:
    if value is None:
        return "n/a"
    if value < 0.001:
        return f"{value * 1_000_000:.2f} us"
    if value < 1.0:
        return f"{value * 1_000:.2f} ms"
    return f"{value:.3f} s"


def _format_io(name: str, value: int) -> str:
    if name.endswith("_bytes"):
        return _format_bytes(value)
    return f"{value:,}"


def _format_bytes(value: int) -> str:
    units = ("B", "KiB", "MiB", "GiB")
    scaled = float(value)
    unit = units[0]
    for unit in units:
        if scaled < 1024.0 or unit == units[-1]:
            break
        scaled /= 1024.0
    return f"{scaled:.2f} {unit}"


def _humanize_name(name: str) -> str:
    return name.replace("_", " ")
