from __future__ import annotations

import threading
import time
from collections.abc import Callable
from dataclasses import dataclass, field
from pathlib import Path

import psutil


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
        self._sample()
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
            cpu_percent = (latest.cpu_time_s - first.cpu_time_s) / runtime * 100.0

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
            if not self._sample():
                break

    def _sample(self) -> bool:
        try:
            self._samples.append(_read_process_snapshot(self.pid))
            return True
        except (OSError, psutil.Error):
            return False


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
