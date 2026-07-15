from __future__ import annotations

from dataclasses import dataclass, field

from .types import Operation


@dataclass(slots=True)
class Histogram:
    count: int = 0
    total: float = 0.0
    minimum: float | None = None
    maximum: float | None = None

    def observe(self, value: float) -> None:
        self.count += 1
        self.total += value
        self.minimum = value if self.minimum is None else min(self.minimum, value)
        self.maximum = value if self.maximum is None else max(self.maximum, value)

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
