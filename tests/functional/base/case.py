import unittest
from pathlib import Path

from jstine import AsyncClient, Client, Protocol

from base.artifacts import TestArtifacts
from base.config import Config
from base.server import Server


class TestCase(unittest.IsolatedAsyncioTestCase):
    def setUp(self, skip_server: bool = False, *args, **kwargs):
        super().setUp(*args, **kwargs)

        self.config = Config.load()
        self.artifacts = TestArtifacts.for_nodeid(self.id())
        self.artifacts.clear()

        if not skip_server:
            self.server = Server(
                self.config.server.binary,
                port=self.config.server.port,
                config_path=self.config.server.config_path,
                artifacts=self.artifacts,
            )

            self.addCleanup(self.server.stop)
            self.server.start()

    def artifact_path(self, name: str = "extra.log") -> Path:
        return self.artifacts.path(name)

    def write_artifact(self, text: str, name: str = "extra.log") -> Path:
        path = self.artifact_path(name)
        self.artifacts.append(path, text)
        return path

    def client(self, host: str = "127.0.0.1", port: int | None = None):
        return Client(
            host=host,
            port=self.config.server.port if port is None else port,
            protocol=Protocol.jfp,
        )

    def async_client(self, host: str = "127.0.0.1", port: int | None = None):
        return AsyncClient(
            host=host,
            port=self.config.server.port if port is None else port,
            protocol=Protocol.jfp,
        )
