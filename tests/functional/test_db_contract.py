from __future__ import annotations

from assertpy import assert_that
from ddt import data, ddt, unpack

from base.case import TestCase
from base.harness import ClientHarness


@ddt
class TestDatabaseContract(TestCase):
    def setUp(self):
        super().setUp()
        self.h = ClientHarness(self)

    @data(
        (b"", b""),
        (b"alpha", b"one"),
        (b"space key", b"space value"),
        (b"binary\x00key", b"value\x00\xff"),
        (bytes(range(32)), bytes(range(31, -1, -1))),
    )
    @unpack
    def test_round_trip_contract(self, key, value):
        self.h.assert_missing_get(key=key)
        self.h.assert_exists(key=key, expected=False)

        self.h.assert_set_get(key=key, value=value)
        self.h.assert_exists(key=key, expected=True)
        assert_that(self.h.client.get(key=key)).is_equal_to(value)

        updated = value + b":updated"
        self.h.assert_overwrite(
            key=key,
            value=value,
            updated_value=updated,
        )
        self.h.assert_exists(key=key, expected=True)
        assert_that(self.h.client.get(key=key)).is_equal_to(updated)

        self.h.assert_delete(key=key, expected=True)
        self.h.assert_missing_get(key=key)
        self.h.assert_exists(key=key, expected=False)
        self.h.assert_delete(key=key, expected=False)

    @data(
        (b"db:primary", b"db:secondary"),
    )
    @unpack
    def test_multiple_keys_remain_isolated(self, primary_key, secondary_key):
        self.h.assert_set_get(key=primary_key, value=b"v1")
        self.h.assert_set_get(key=secondary_key, value=b"v2")
        self.h.assert_exists(key=primary_key, expected=True)
        self.h.assert_exists(key=secondary_key, expected=True)

        self.h.assert_overwrite(
            key=primary_key,
            value=b"v1",
            updated_value=b"v1:updated",
        )

        assert_that(self.h.client.get(key=primary_key)).is_equal_to(b"v1:updated")
        assert_that(self.h.client.get(key=secondary_key)).is_equal_to(b"v2")

        self.h.assert_delete(key=primary_key, expected=True)
        self.h.assert_exists(key=primary_key, expected=False)
        self.h.assert_exists(key=secondary_key, expected=True)
        assert_that(self.h.client.get(key=secondary_key)).is_equal_to(b"v2")

    @data(b"db:delete")
    def test_delete_is_idempotent(self, key):
        self.h.assert_delete(key=key, expected=False)
        self.h.assert_missing_get(key=key)

        self.h.client.set(key=key, value=b"value")
        self.h.assert_exists(key=key, expected=True)
        self.h.assert_delete(key=key, expected=True)
        self.h.assert_delete(key=key, expected=False)
        self.h.assert_exists(key=key, expected=False)

    @data((b"", b""))
    @unpack
    def test_empty_key_and_empty_value_are_supported(self, key, value):
        self.h.assert_missing_get(key=key)
        self.h.assert_set_get(key=key, value=value)
        self.h.assert_exists(key=key, expected=True)
        self.h.assert_delete(key=key, expected=True)
        self.h.assert_missing_get(key=key)

    @data(
        (
            b"db:binary",
            bytes([0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF, 0x10, 0x20]),
            bytes([0xFF, 0xFE, 0x80, 0x7F, 0x01, 0x00, 0xAA, 0x55]),
        ),
    )
    @unpack
    def test_binary_values_round_trip_exactly(self, key, value, updated):
        self.h.assert_set_get(key=key, value=value)
        assert_that(self.h.client.get(key=key)).is_equal_to(value)

        self.h.assert_overwrite(key=key, value=value, updated_value=updated)
        assert_that(self.h.client.get(key=key)).is_equal_to(updated)
        self.h.assert_delete(key=key, expected=True)

    def test_large_value_round_trip_exactly(self):
        key = b"db:large:value"
        value = bytes((i % 256 for i in range(768)))
        updated = bytes(((255 - i) % 256 for i in range(768)))

        self.h.assert_set_get(key=key, value=value)
        assert_that(self.h.client.get(key=key)).is_equal_to(value)

        self.h.assert_overwrite(key=key, value=value, updated_value=updated)
        assert_that(self.h.client.get(key=key)).is_equal_to(updated)
        self.h.assert_delete(key=key, expected=True)

    def test_long_key_round_trip_exactly(self):
        key = b"db:" + bytes((65 + (i % 26) for i in range(256)))
        value = b"long-key-value"

        self.h.assert_set_get(key=key, value=value)
        assert_that(self.h.client.get(key=key)).is_equal_to(value)
        self.h.assert_exists(key=key, expected=True)
        self.h.assert_delete(key=key, expected=True)

    @data(b"db:recreate")
    def test_recreating_a_key_replaces_the_previous_value(self, key):
        self.h.client.set(key=key, value=b"first")
        self.h.assert_exists(key=key, expected=True)
        assert_that(self.h.client.get(key=key)).is_equal_to(b"first")

        self.h.client.delete(key=key)
        self.h.assert_exists(key=key, expected=False)
        self.h.assert_missing_get(key=key)

        self.h.client.set(key=key, value=b"second")
        self.h.assert_exists(key=key, expected=True)
        assert_that(self.h.client.get(key=key)).is_equal_to(b"second")

    def test_overwrite_chain_keeps_latest_value(self):
        key = b"db:chain"
        values = [b"one", b"two", b"three", b"four"]

        for value in values:
            self.h.client.set(key=key, value=value)
            assert_that(self.h.client.get(key=key)).is_equal_to(value)

        self.h.assert_exists(key=key, expected=True)
        self.h.assert_delete(key=key, expected=True)
        self.h.assert_missing_get(key=key)

    @data(tuple(f"db:key:{i}".encode("ascii") for i in range(12)))
    def test_write_many_keys_and_delete_subset(self, keys):
        for index, key in enumerate(keys):
            self.h.client.set(key=key, value=f"value:{index}".encode("ascii"))

        for index, key in enumerate(keys):
            self.h.assert_exists(key=key, expected=True)
            assert_that(self.h.client.get(key=key)).is_equal_to(
                f"value:{index}".encode("ascii")
            )

        for key in keys[::2]:
            self.h.assert_delete(key=key, expected=True)

        for index, key in enumerate(keys):
            if index % 2 == 0:
                self.h.assert_exists(key=key, expected=False)
                self.h.assert_missing_get(key=key)
            else:
                self.h.assert_exists(key=key, expected=True)
                assert_that(self.h.client.get(key=key)).is_equal_to(
                    f"value:{index}".encode("ascii")
                )
