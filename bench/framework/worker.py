from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass
from typing import TypeAlias

from .recorder import Recorder
from .types import BenchmarkOperation, Client


WorkerFactory: TypeAlias = Callable[[Client, int, Recorder], BenchmarkOperation]


@dataclass(frozen=True, slots=True)
class WorkerDefinition:
    tag: str
    default: int
    factory: WorkerFactory


@dataclass(frozen=True, slots=True)
class WorkerInstance:
    definition: WorkerDefinition
    index: int
