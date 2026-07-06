from __future__ import annotations

import os
import signal
import socket
import subprocess
import threading
import time
from pathlib import Path

from base.artifacts import TestArtifacts
from base.process_metrics import ProcessMetrics


_SAMPLE_INTERVAL_SECONDS = 0.25


class Server:
    def __init__(
        self,
        binary: str | Path,
        port: int,
        config_path: str | Path,
        artifacts: TestArtifacts | None = None,
    ) -> None:
        self.binary = Path(binary)
        self.port = port
        self.config_path = Path(config_path)
        self.artifacts = artifacts
        self.process: subprocess.Popen[str] | None = None
        self._stdout = None
        self._stderr = None
        self._metrics = ProcessMetrics(
            command=[
                str(self.binary),
                "-c",
                str(self.config_path),
                "--port",
                str(self.port),
            ],
            sample_interval_s=_SAMPLE_INTERVAL_SECONDS,
        )
        self._metrics_thread: threading.Thread | None = None
        self._metrics_stop = threading.Event()

    def _open_logs(self) -> tuple[object | None, object | None]:
        if self.artifacts is None:
            return None, None

        self.artifacts.root.mkdir(parents=True, exist_ok=True)
        stdout = self.artifacts.server_stdout.open("a", encoding="utf-8")
        stderr = self.artifacts.server_stderr.open("a", encoding="utf-8")
        return stdout, stderr

    def start(self, timeout: float = 10.0) -> None:
        if self.process is not None and self.process.poll() is None:
            return

        self._metrics_stop.clear()
        self._stdout, self._stderr = self._open_logs()
        self.process = subprocess.Popen(
            self._metrics.command,
            stdout=self._stdout or subprocess.DEVNULL,
            stderr=self._stderr or subprocess.DEVNULL,
            text=True,
            start_new_session=True,
        )
        self._metrics.start(self.process.pid)
        self._metrics_thread = threading.Thread(
            target=self._metrics_loop,
            name="server-metrics",
            daemon=True,
        )
        self._metrics_thread.start()
        try:
            self._wait_until_ready(timeout)
            self._metrics.sample()
        except Exception:
            self.stop()
            raise

    def _metrics_loop(self) -> None:
        while not self._metrics_stop.wait(_SAMPLE_INTERVAL_SECONDS):
            if self.process is None:
                return
            if self.process.poll() is not None:
                self._metrics.sample()
                return
            self._metrics.sample()

    def _wait_until_ready(self, timeout: float) -> None:
        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:
            if self.process is None:
                raise RuntimeError("Server process was not started")

            if self.process.poll() is not None:
                raise RuntimeError("Server exited before becoming ready")

            try:
                with socket.create_connection(("127.0.0.1", self.port), timeout=0.2):
                    return
            except OSError:
                time.sleep(0.1)

        raise TimeoutError(f"Server did not become ready on port {self.port}")

    def stop(self) -> None:
        if self.process is None:
            return

        self._metrics.sample()
        self._metrics.finish()
        self._metrics_stop.set()
        if self._metrics_thread is not None:
            self._metrics_thread.join(timeout=5.0)
            self._metrics_thread = None

        if self.process.poll() is None:
            try:
                os.killpg(self.process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            except PermissionError:
                self.process.terminate()

            try:
                self.process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(self.process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                except PermissionError:
                    self.process.kill()
                self.process.wait(timeout=5.0)

        if self.artifacts is not None:
            self._metrics.write_json(self.artifacts.server_metrics)
            self._metrics.write_report(self.artifacts.server_metrics_report)

        if self._stdout is not None:
            self._stdout.close()
            self._stdout = None

        if self._stderr is not None:
            self._stderr.close()
            self._stderr = None

        self.process = None
