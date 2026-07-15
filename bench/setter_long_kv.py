from framework import Benchmark, BenchmarkOperation, Client, Recorder, Random, Sequence

bench = Benchmark("setter-long-kv")


@bench.worker(tag="setter", default=50)
def setter(client: Client, worker_id: int, record: Recorder) -> BenchmarkOperation:
    n = 10000
    vn = 1000

    key_seq = Sequence.seq_generator(f"key:bench:{worker_id}", n)
    value_seq = Random.word_generator(vn, f"value:bench:{worker_id}")

    def operation() -> None:
        with record.set():
            ok = client.set(next(key_seq), next(value_seq))
            assert ok

    return operation


if __name__ == "__main__":
    bench.main()
