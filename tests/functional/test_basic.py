from base.case import TestCase
from base.harness import ClientHarness


class TestBasic(TestCase):
    def setUp(self):
        super().setUp()
        self.h = ClientHarness(self)

    def test_ping(self):
        """ Test the ping functionality """
        self.h.assert_ping(b"Hello world!")

    def test_set_get(self):
        """ Test if get returns value added by set """
        self.h.assert_set_get(key=b"my:key", value=b"Hello world!")

    def test_exists(self):
        """ Test if exists returns correct value """
        key = b"my:key"

        self.h.assert_exists(key=key, expected=False)
        self.h.client.set(key=key, value=b"Hello world!")
        self.h.assert_exists(key=key, expected=True)

    def test_set_update_value(self):
        """ Test if set updates previous value """
        self.h.assert_overwrite(
            key=b"my:key",
            value=b"Hello world!",
            updated_value=b"Updated value",
        )

    def test_get_on_non_existing_value(self):
        """ Test if get returns None for non-existing key """
        self.h.assert_missing_get(key=b"non:existent:key")
