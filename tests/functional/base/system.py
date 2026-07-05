from __future__ import annotations

from enum import Enum
import sys


class System(Enum):
    linux = "linux"
    darwin = "darwin"

    @classmethod
    def os(cls) -> "System":
        if sys.platform.startswith("linux"):
            return cls.linux
        if sys.platform == "darwin":
            return cls.darwin

        raise RuntimeError(
            f"Unsupported test platform: {sys.platform}. "
            "Functional tests are only supported on Linux and macOS."
        )
