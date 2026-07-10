from framework import Benchmark, BenchmarkOperation, Client, Recorder


bench = Benchmark("mixed")


@bench.worker(tag="setter", default=1)
def setter(client: Client, worker_id: int, record: Recorder) -> BenchmarkOperation:
    key = f"key:bench:mixed:{worker_id}"
    value = f"value:bench:mixed:{worker_id}"

    def operation() -> None:
        with record.set():
            ok = client.set(key, value)
            assert ok

    return operation


@bench.worker(tag="getter", default=50)
def getter(client: Client, worker_id: int, record: Recorder) -> BenchmarkOperation:
    key = f"key:bench:not_existing:{worker_id}"

    def operation() -> None:
        with record.get():
            value = client.get(key)
            assert value is None

    return operation


@bench.worker(tag="set_get", default=0)
def set_get(client: Client, worker_id: int, record: Recorder) -> BenchmarkOperation:
    key = f"key:bench:set_get:{worker_id}"
    value = f"value:bench:set_get:{worker_id}"

    def operation() -> None:
        with record.set():
            ok = client.set(key, value)
            assert ok

        with record.get():
            assert client.get(key) == value.encode()

    return operation


if __name__ == "__main__":
    bench.main()
