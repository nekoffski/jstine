from __future__ import annotations


def coerce_bytes(value: bytes | bytearray | memoryview | str | int | float) -> bytes:
    if isinstance(value, bytes):
        return value
    if isinstance(value, bytearray):
        return bytes(value)
    if isinstance(value, memoryview):
        return value.tobytes()
    if isinstance(value, str):
        return value.encode("utf-8")
    if isinstance(value, bool):
        raise TypeError("bool is not a supported binary value")
    if isinstance(value, (int, float)):
        return str(value).encode("ascii")
    raise TypeError(f"Unsupported value type: {type(value).__name__}")
