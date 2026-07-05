from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


LOGS_ROOT = Path.cwd() / "logs"


def canonical_test_name(name: str) -> str:
    name = name.replace("::", ".").replace("/", ".").replace("\\", ".")
    return re.sub(r"\.py(?=\.|$)", "", name)


def _sanitize(name: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_.-]+", "_", name)
    return cleaned.strip("._") or "test"


def test_log_dir(name: str, root: Path = LOGS_ROOT) -> Path:
    path = root / _sanitize(canonical_test_name(name))
    path.mkdir(parents=True, exist_ok=True)
    return path


@dataclass(slots=True)
class TestArtifacts:
    root: Path

    @classmethod
    def for_nodeid(cls, nodeid: str, root: Path = LOGS_ROOT) -> "TestArtifacts":
        return cls(root=test_log_dir(nodeid, root=root))

    @property
    def server_stdout(self) -> Path:
        return self.root / "server.stdout.log"

    @property
    def server_stderr(self) -> Path:
        return self.root / "server.stderr.log"

    @property
    def pytest_log(self) -> Path:
        return self.root / "pytest.log"

    def append(self, path: Path, text: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("a", encoding="utf-8") as fh:
            fh.write(text)
            if text and not text.endswith("\n"):
                fh.write("\n")

