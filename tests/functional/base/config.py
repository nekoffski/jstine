from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import tomllib


ROOT_DIR = Path(__file__).resolve().parents[3]
DEFAULT_CONFIG_PATH = ROOT_DIR / "tests" / "conf" / "functionals.toml"
DEFAULT_SERVER_CONFIG_PATH = ROOT_DIR / "conf" / "config-template.toml"


class ConfigError(ValueError):
    pass


@dataclass(slots=True)
class ServerConfig:
    binary: Path
    config_path: Path = DEFAULT_SERVER_CONFIG_PATH
    port: int = 9991


@dataclass(slots=True)
class Config:
    server: ServerConfig

    @classmethod
    def load(cls, path: Path | str = DEFAULT_CONFIG_PATH) -> "Config":
        config_path = Path(path)
        if not config_path.is_file():
            raise ConfigError(f"Test config file does not exist: {config_path}")

        raw = config_path.read_text(encoding="utf-8").replace(
            "{cwd}", str(Path.cwd())
        )
        data = tomllib.loads(raw)

        server_data = data.get("server")
        if not isinstance(server_data, dict):
            raise ConfigError("Test config must contain a [server] section")

        binary_value = server_data.get("binary")
        if not isinstance(binary_value, str) or not binary_value:
            raise ConfigError("Test config must define [server].binary")

        binary = Path(binary_value)
        if not binary.is_absolute():
            binary = (ROOT_DIR / binary).resolve()

        config_value = server_data.get("config_path", DEFAULT_SERVER_CONFIG_PATH)
        if not isinstance(config_value, (str, Path)):
            raise ConfigError("[server].config_path must be a string")

        config_file = Path(config_value)
        if not config_file.is_absolute():
            config_file = (ROOT_DIR / config_file).resolve()

        port_value = server_data.get("port", 9991)
        if not isinstance(port_value, int):
            raise ConfigError("[server].port must be an integer")

        return cls(
            server=ServerConfig(
                binary=binary,
                config_path=config_file,
                port=port_value,
            )
        )

