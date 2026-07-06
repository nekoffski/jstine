from __future__ import annotations

import json
import os
import subprocess
import threading
import time
import psutil
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Sequence

from base.system import System


_CLOCK_TICKS_PER_SECOND = os.sysconf(os.sysconf_names["SC_CLK_TCK"])
_PAGE_SIZE_KIB = os.sysconf("SC_PAGE_SIZE") // 1024


class CollectorKind(Enum):
    linux = "linux"
    darwin = "darwin"

    @classmethod
    def from_system(cls, system: System) -> "CollectorKind":
        return cls[system.value]


@dataclass(slots=True)
class ProcessSnapshot:
    timestamp_monotonic: float
    user_time_s: float
    system_time_s: float
    rss_kib: int
    peak_rss_kib: int


class ProcessMetrics:
    def __init__(
        self,
        command: Sequence[str],
        sample_interval_s: float = 0.25,
        system: System | None = None,
    ) -> None:
        self.command = list(command)
        self.system = system or System.os()
        self.collector_kind = CollectorKind.from_system(self.system)
        self.sample_interval_s = sample_interval_s
        self.pid: int | None = None
        self.started_monotonic: float | None = None
        self.stopped_monotonic: float | None = None
        self.samples = 0
        self.latest_snapshot: ProcessSnapshot | None = None
        self._peak_rss_kib: int | None = None
        self._lock = threading.Lock()

    def start(self, pid: int) -> None:
        self.pid = pid
        self.started_monotonic = time.monotonic()
        self.sample()

    def sample(self) -> None:
        if self.pid is None:
            return

        try:
            snapshot = self._read_snapshot(self.pid)
        except OSError:
            return

        with self._lock:
            self.latest_snapshot = snapshot
            self.samples += 1
            if self._peak_rss_kib is None:
                self._peak_rss_kib = snapshot.peak_rss_kib
            else:
                self._peak_rss_kib = max(
                    self._peak_rss_kib, snapshot.peak_rss_kib)

    def finish(self) -> None:
        self.stopped_monotonic = time.monotonic()

    def as_dict(self) -> dict[str, object]:
        with self._lock:
            snapshot = self.latest_snapshot
            samples = self.samples
            peak_rss_kib = self._peak_rss_kib

        started = self.started_monotonic
        stopped = self.stopped_monotonic or time.monotonic()
        runtime_s = (
            max(0.0, stopped - started) if started is not None else None
        )

        user_time_s = snapshot.user_time_s if snapshot is not None else None
        system_time_s = snapshot.system_time_s if snapshot is not None else None
        cpu_time_s = (
            user_time_s + system_time_s
            if user_time_s is not None and system_time_s is not None
            else None
        )
        cpu_percent = (
            (cpu_time_s / runtime_s * 100.0)
            if runtime_s and runtime_s > 0 and cpu_time_s is not None
            else None
        )

        return {
            "os": self.system.value,
            "collector": self.collector_kind.value,
            "pid": self.pid,
            "command": self.command,
            "sample_interval_s": self.sample_interval_s,
            "samples": samples,
            "runtime_s": runtime_s,
            "cpu_time_s": cpu_time_s,
            "user_time_s": user_time_s,
            "system_time_s": system_time_s,
            "cpu_percent_avg": cpu_percent,
            "rss_kib": snapshot.rss_kib if snapshot is not None else None,
            "peak_rss_kib": peak_rss_kib,
        }

    def render_report(self) -> str:
        data = self.as_dict()
        runtime_s = data["runtime_s"]
        cpu_time_s = data["cpu_time_s"]
        user_time_s = data["user_time_s"]
        system_time_s = data["system_time_s"]
        cpu_percent = data["cpu_percent_avg"]
        rss_kib = data["rss_kib"]
        peak_rss_kib = data["peak_rss_kib"]

        lines = [
            "Process metrics",
            f"  os: {data['os']}",
            f"  collector: {data['collector']}",
            f"  pid: {data['pid']}",
            f"  command: {' '.join(str(part) for part in self.command)}",
            f"  samples: {data['samples']} @ {self.sample_interval_s:.3f}s",
        ]

        if runtime_s is not None:
            lines.append(f"  runtime: {runtime_s:.3f}s")

        lines.append("  cpu:")
        if user_time_s is not None:
            lines.append(f"    user: {user_time_s:.3f}s")
        if system_time_s is not None:
            lines.append(f"    system: {system_time_s:.3f}s")
        if cpu_time_s is not None:
            lines.append(f"    total: {cpu_time_s:.3f}s")
        if cpu_percent is not None:
            lines.append(f"    average: {cpu_percent:.1f}% of one core")

        lines.extend(
            [
                "  memory:",
                f"    rss: {self._format_kib(rss_kib)}",
                f"    peak rss: {self._format_kib(peak_rss_kib)}",
            ]
        )
        return "\n".join(lines) + "\n"

    def write_json(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            json.dumps(self.as_dict(), indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )

    def write_report(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(self.render_report(), encoding="utf-8")

    @staticmethod
    def _format_kib(value: object) -> str:
        if not isinstance(value, int):
            return "n/a"

        mebibytes = value / 1024.0
        return f"{value} KiB ({mebibytes:.2f} MiB)"

    @staticmethod
    def _read_snapshot(pid: int) -> ProcessSnapshot:
        system = System.os()
        if system == System.linux:
            return ProcessMetrics._read_linux_snapshot(pid)
        if system == System.darwin:
            return ProcessMetrics._read_darwin_snapshot(pid)

        raise RuntimeError(f"Unsupported test platform: {system.value}")

    @staticmethod
    def _read_linux_snapshot(pid: int) -> ProcessSnapshot:
        stat_path = Path(f"/proc/{pid}/stat")
        status_path = Path(f"/proc/{pid}/status")

        stat_parts = stat_path.read_text(encoding="utf-8").strip()
        end_comm = stat_parts.rfind(") ")
        if end_comm == -1:
            raise OSError(f"Malformed /proc/{pid}/stat entry")

        fields = stat_parts[end_comm + 2:].split()
        if len(fields) < 22:
            raise OSError(f"Unexpected /proc/{pid}/stat field count")

        user_time_s = int(fields[11]) / _CLOCK_TICKS_PER_SECOND
        system_time_s = int(fields[12]) / _CLOCK_TICKS_PER_SECOND
        rss_kib = int(fields[21]) * _PAGE_SIZE_KIB
        peak_rss_kib = rss_kib

        for line in status_path.read_text(encoding="utf-8").splitlines():
            if line.startswith("VmRSS:"):
                rss_kib = int(line.split()[1])
            elif line.startswith("VmHWM:"):
                peak_rss_kib = int(line.split()[1])

        return ProcessSnapshot(
            timestamp_monotonic=time.monotonic(),
            user_time_s=user_time_s,
            system_time_s=system_time_s,
            rss_kib=rss_kib,
            peak_rss_kib=peak_rss_kib,
        )

    @staticmethod
    def _read_darwin_snapshot(pid: int) -> ProcessSnapshot:
        p = psutil.Process(pid)

        cpu = p.cpu_times()
        mem = p.memory_info()

        return ProcessSnapshot(
            timestamp_monotonic=time.monotonic(),
            user_time_s=cpu.user,
            system_time_s=cpu.system,
            rss_kib=mem.rss // 1024,
            peak_rss_kib=mem.rss // 1024,  # psutil does not expose peak RSS on macOS
        )


def _parse_cpu_time(value: str) -> float:
    value = value.strip()
    if not value:
        raise OSError("Empty CPU time value")

    days = 0
    if "-" in value:
        day_part, value = value.split("-", 1)
        days = int(day_part)

    chunks = value.split(":")
    if len(chunks) == 3:
        hours, minutes, seconds = chunks
    elif len(chunks) == 2:
        hours = "0"
        minutes, seconds = chunks
    elif len(chunks) == 1:
        hours = "0"
        minutes = "0"
        seconds = chunks[0]
    else:
        raise OSError(f"Unrecognized CPU time format: {value!r}")

    total_seconds = (
        days * 24 * 3600
        + int(hours) * 3600
        + int(minutes) * 60
        + float(seconds)
    )
    return total_seconds
