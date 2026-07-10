from framework import Benchmark, BenchmarkOperation, Client, Recorder


bench = Benchmark("getter")


@bench.worker(tag="getter", default=50)
def getter(client: Client, worker_id: int, record: Recorder) -> BenchmarkOperation:
    key = f"key:bench:not_existing:{worker_id}"

    def operation() -> None:
        with record.get():
            value = client.get(key)
            assert value is None

    return operation


if __name__ == "__main__":
    bench.main()
