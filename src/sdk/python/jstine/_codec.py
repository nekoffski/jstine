from abc import ABC, abstractmethod

from ._proto import Protocol, RequestKind


class Codec(ABC):
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
