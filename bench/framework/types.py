from __future__ import annotations

from collections.abc import Callable
from typing import Literal, TypeAlias

import jstine


Operation: TypeAlias = Literal["set", "get"]
BenchmarkOperation: TypeAlias = Callable[[], None]
Client = jstine.Client
BenchmarkInitHook: TypeAlias = Callable[[Client, dict[str, int]], None]
