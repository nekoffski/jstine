from framework import Benchmark, BenchmarkOperation, Client, Recorder

bench = Benchmark("setter")


@bench.worker(tag="setter", default=50)
def setter(client: Client, worker_id: int, record: Recorder) -> BenchmarkOperation:
    n = 10000

    def key_generator() -> str:
        i = 0
        while True:
            yield f"key:bench:{worker_id}:{i}"
            i = (i + 1) % n

    key = key_generator()
    value = f"value:bench:{worker_id}"

    def operation() -> None:
        with record.set():
            ok = client.set(next(key), value)
            assert ok

    return operation


if __name__ == "__main__":
    bench.main()
