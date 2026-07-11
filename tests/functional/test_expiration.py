from base.case import TestCase
from base.harness import ClientHarness


class TestExpiration(TestCase):
    def setUp(self):
        super().setUp()
        self.h = ClientHarness(self)

    def _default_expiration(self) -> float:
        return self.server_config().storage.default_expiration_seconds

    def test_default_expiration_policy(self):
        """ Test that value is reaped after default expiration period is over """
        key = b"test:key"
        self.h.assert_exists(key=key, expected=False)
        self.h.assert_set(key, value="some:test:value")
        self.h.assert_exists(key=key, expected=True)
        self.sleep(self._default_expiration())
        self.h.assert_exists(key=key, expected=False)
