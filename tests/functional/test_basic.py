from base.case import TestCase

from assertpy import assert_that


class TestBasic(TestCase):
    def setUp(self):
        super().setUp()

        self.c = self.client()
        self.c.connect()

    def tearDown(self):
        self.c.close()

    def test_ping(self):
        """ Test the ping functionality """
        payload = b"Hello world!"

        pong = self.c.ping(payload=payload)
        assert_that(pong).is_equal_to(payload)

    def test_set_get(self):
        """ Test if get returns value added by set """
        key = b"my:key"
        value = b"Hello world!"

        resp = self.c.set(key=key, value=value)
        assert_that(resp).is_true()

        got = self.c.get(key=key)
        assert_that(got).is_equal_to(value)

    def test_exists(self):
        """ Test if exists returns correct value """
        key = b"my:key"

        exists = self.c.exists(key=key)
        assert_that(exists).is_false()

        resp = self.c.set(key=key, value=b"Hello world!")
        assert_that(resp).is_true()

        exists = self.c.exists(key=key)
        assert_that(exists).is_true()

    def test_set_update_value(self):
        """ Test if set updates previous value """
        key = b"my:key"
        value = b"Hello world!"

        resp = self.c.set(key=key, value=value)
        assert_that(resp).is_true()

        resp = self.c.get(key=key)
        assert_that(resp).is_equal_to(value)

        new_value = b"Updated value"
        resp = self.c.set(key=key, value=new_value)
        assert_that(resp).is_true()

        got = self.c.get(key=key)
        assert_that(got).is_equal_to(new_value)

    def test_get_on_non_existing_value(self):
        """ Test if get returns None for non-existing key """
        key = b"non:existent:key"
        got = self.c.get(key=key)
        assert_that(got).is_none()
