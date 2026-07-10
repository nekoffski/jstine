import jstine
from base.bench import Benchmark, repeat


class BenchmarkSetGet(Benchmark):
    async def test_set_single_client_static_kv(self):
        self.add_setter(
            key_generator=repeat("key:bench"),
            value_generator=repeat("value:bench")
        )
        await self.run_for(duration=15)

    async def test_set_multiple_clients_static_kv(self):
        for i in range(50):
            self.add_setter(
                key_generator=repeat(f"key:bench:{i}"),
                value_generator=repeat(f"value:bench:{i}")
            )
        await self.run_for(duration=15)
