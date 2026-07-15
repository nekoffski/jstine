import string
import random


class Random:
    ALPHABET = string.ascii_lowercase + string.ascii_uppercase + string.digits

    @classmethod
    def string(cls, length: int) -> str:
        return "".join(random.choice(cls.ALPHABET) for _ in range(length))

    @classmethod
    def word_generator(cls, size: int, prefix: str | None = None):
        if prefix is not None:
            size -= len(prefix) + 1
        while True:
            yield f"{prefix}:{cls.string(size)}" if prefix else cls.string(size)


class Sequence:
    @classmethod
    def seq_generator(cls, prefix: str, max_index: int | None = None):
        i = 0
        while True:
            yield f"{prefix}:{i}"
            i += 1
            if max_index and i >= max_index:
                i = 0
