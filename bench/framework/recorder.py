from __future__ import annotations

import contextlib
import time
from collections.abc import Iterator

from .metrics import Metrics


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
