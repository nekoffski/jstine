from abc import ABC, abstractmethod
from typing import TypeAlias

from ._proto import FieldType, Protocol, RequestKind

RequestField: TypeAlias = tuple[FieldType, bytes]


class Codec(ABC):
    @abstractmethod
    def pack_request(
        self, kind: RequestKind, fields: list[RequestField]
    ) -> bytes: ...

    @abstractmethod
    def pack_ping(self, payload: bytes) -> bytes: ...

    @abstractmethod
    def unpack_response(self, data: bytes) -> bytes: ...


def make_codec(protocol: Protocol) -> Codec:
    if protocol == Protocol.jfp:
        from ._jfp import JFPCodec
        return JFPCodec()
    raise NotImplementedError(
        f"Protocol {protocol.name!r} is not yet implemented")
