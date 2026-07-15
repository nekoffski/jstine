from __future__ import annotations

import time
from dataclasses import dataclass
from pathlib import Path

from base.artifacts import TestArtifacts
from base.config import Config, ServerRuntimeConfig
from base.server import Server
from jstine import AsyncClient, Client, Protocol


@dataclass
class Context:
    test_config: Config
    runtime_config: ServerRuntimeConfig
    artifacts: TestArtifacts
    server: Server | None = None

    def sleep(self, seconds: float) -> None:
        time.sleep(seconds)

    def config(self) -> Config:
        return self.test_config

    def server_config(self) -> ServerRuntimeConfig:
        return self.runtime_config

    def artifact_path(self, name: str = "extra.log") -> Path:
        return self.artifacts.path(name)

    def write_artifact(self, text: str, name: str = "extra.log") -> Path:
        path = self.artifact_path(name)
        self.artifacts.append(path, text)
        return path

    def client(self, host: str = "127.0.0.1", port: int | None = None) -> Client:
        return Client(
            host=host,
            port=self.config().server.port if port is None else port,
            protocol=Protocol.jfp,
        )

    def async_client(
        self,
        host: str = "127.0.0.1",
        port: int | None = None,
    ) -> AsyncClient:
        return AsyncClient(
            host=host,
            port=self.config().server.port if port is None else port,
            protocol=Protocol.jfp,
        )
