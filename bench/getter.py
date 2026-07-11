from framework import Benchmark, BenchmarkOperation, Client, Recorder


bench = Benchmark("getter")


@bench.init
def init_hook(client: Client, config: dict[str, int]) -> None:
    for i in range(config.get("getter", 0)):
        key = f"key:bench:existing:{i}"
        value = f"value:bench:existing:{i}"
        ok = client.set(key, value)
        assert ok


@bench.worker(tag="getter", default=50)
def getter(client: Client, worker_id: int, record: Recorder) -> BenchmarkOperation:
    key = f"key:bench:existing:{worker_id}"

    def operation() -> None:
        with record.get():
            value = client.get(key)
            assert value is not None

    return operation


if __name__ == "__main__":
    bench.main()
