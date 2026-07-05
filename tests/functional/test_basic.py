from base.case import TestCase

from assertpy import assert_that


class TestBasic(TestCase):
    def test_ping(self):
        with self.client() as c:
            payload = b"Hello world!"

            pong = c.ping(payload=payload)
            assert_that(pong).is_equal_to(payload)

    def test_set_get(self):
        with self.client() as c:
            key = b"my:key"
            value = b"Hello world!"

            resp = c.set(key=key, value=value)
            assert_that(resp).is_true()

            got = c.get(key=key)
            assert_that(got).is_equal_to(value)
