from base.case import TestCase

from assertpy import assert_that


class TestBasic(TestCase):
    async def test_ping(self):
        async with self.async_client() as c:
            payload = b"Hello world!"

            pong = await c.ping(payload=payload)
            assert_that(pong).is_equal_to(payload)
