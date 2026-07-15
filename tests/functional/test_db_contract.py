from __future__ import annotations

import pytest
from assertpy import assert_that

from base.harness import ClientHarness


@pytest.mark.parametrize(
    ("key", "value"),
    [
        (b"", b""),
        (b"alpha", b"one"),
        (b"space key", b"space value"),
        (b"binary\x00key", b"value\x00\xff"),
        (bytes(range(32)), bytes(range(31, -1, -1))),
    ],
    ids=["empty", "ascii", "spaces", "binary", "range"],
)
def test_round_trip_contract(
    client_harness: ClientHarness,
    key: bytes,
    value: bytes,
) -> None:
    client_harness.assert_missing_get(key=key)
    client_harness.assert_exists(key=key, expected=False)

    client_harness.assert_set_get(key=key, value=value)
    client_harness.assert_exists(key=key, expected=True)
    assert_that(client_harness.client.get(key=key)).is_equal_to(value)

    updated = value + b":updated"
    client_harness.assert_overwrite(
        key=key,
        value=value,
        updated_value=updated,
    )
    client_harness.assert_exists(key=key, expected=True)
    assert_that(client_harness.client.get(key=key)).is_equal_to(updated)

    client_harness.assert_delete(key=key, expected=True)
    client_harness.assert_missing_get(key=key)
    client_harness.assert_exists(key=key, expected=False)
    client_harness.assert_delete(key=key, expected=False)


@pytest.mark.parametrize(
    ("primary_key", "secondary_key"),
    [
        (b"db:primary", b"db:secondary"),
    ],
    ids=["primary-secondary"],
)
def test_multiple_keys_remain_isolated(
    client_harness: ClientHarness,
    primary_key: bytes,
    secondary_key: bytes,
) -> None:
    client_harness.assert_set_get(key=primary_key, value=b"v1")
    client_harness.assert_set_get(key=secondary_key, value=b"v2")
    client_harness.assert_exists(key=primary_key, expected=True)
    client_harness.assert_exists(key=secondary_key, expected=True)

    client_harness.assert_overwrite(
        key=primary_key,
        value=b"v1",
        updated_value=b"v1:updated",
    )

    assert_that(client_harness.client.get(key=primary_key)).is_equal_to(b"v1:updated")
    assert_that(client_harness.client.get(key=secondary_key)).is_equal_to(b"v2")

    client_harness.assert_delete(key=primary_key, expected=True)
    client_harness.assert_exists(key=primary_key, expected=False)
    client_harness.assert_exists(key=secondary_key, expected=True)
    assert_that(client_harness.client.get(key=secondary_key)).is_equal_to(b"v2")


@pytest.mark.parametrize("key", [b"db:delete"], ids=["delete"])
def test_delete_is_idempotent(client_harness: ClientHarness, key: bytes) -> None:
    client_harness.assert_delete(key=key, expected=False)
    client_harness.assert_missing_get(key=key)

    client_harness.client.set(key=key, value=b"value")
    client_harness.assert_exists(key=key, expected=True)
    client_harness.assert_delete(key=key, expected=True)
    client_harness.assert_delete(key=key, expected=False)
    client_harness.assert_exists(key=key, expected=False)


@pytest.mark.parametrize(("key", "value"), [(b"", b"")], ids=["empty"])
def test_empty_key_and_empty_value_are_supported(
    client_harness: ClientHarness,
    key: bytes,
    value: bytes,
) -> None:
    client_harness.assert_missing_get(key=key)
    client_harness.assert_set_get(key=key, value=value)
    client_harness.assert_exists(key=key, expected=True)
    client_harness.assert_delete(key=key, expected=True)
    client_harness.assert_missing_get(key=key)


@pytest.mark.parametrize(
    ("key", "value", "updated"),
    [
        (
            b"db:binary",
            bytes([0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF, 0x10, 0x20]),
            bytes([0xFF, 0xFE, 0x80, 0x7F, 0x01, 0x00, 0xAA, 0x55]),
        ),
    ],
    ids=["binary"],
)
def test_binary_values_round_trip_exactly(
    client_harness: ClientHarness,
    key: bytes,
    value: bytes,
    updated: bytes,
) -> None:
    client_harness.assert_set_get(key=key, value=value)
    assert_that(client_harness.client.get(key=key)).is_equal_to(value)

    client_harness.assert_overwrite(key=key, value=value, updated_value=updated)
    assert_that(client_harness.client.get(key=key)).is_equal_to(updated)
    client_harness.assert_delete(key=key, expected=True)


def test_large_value_round_trip_exactly(client_harness: ClientHarness) -> None:
    key = b"db:large:value"
    value = bytes((i % 256 for i in range(768)))
    updated = bytes(((255 - i) % 256 for i in range(768)))

    client_harness.assert_set_get(key=key, value=value)
    assert_that(client_harness.client.get(key=key)).is_equal_to(value)

    client_harness.assert_overwrite(key=key, value=value, updated_value=updated)
    assert_that(client_harness.client.get(key=key)).is_equal_to(updated)
    client_harness.assert_delete(key=key, expected=True)


def test_long_key_round_trip_exactly(client_harness: ClientHarness) -> None:
    key = b"db:" + bytes((65 + (i % 26) for i in range(256)))
    value = b"long-key-value"

    client_harness.assert_set_get(key=key, value=value)
    assert_that(client_harness.client.get(key=key)).is_equal_to(value)
    client_harness.assert_exists(key=key, expected=True)
    client_harness.assert_delete(key=key, expected=True)


@pytest.mark.parametrize("key", [b"db:recreate"], ids=["recreate"])
def test_recreating_a_key_replaces_the_previous_value(
    client_harness: ClientHarness,
    key: bytes,
) -> None:
    client_harness.client.set(key=key, value=b"first")
    client_harness.assert_exists(key=key, expected=True)
    assert_that(client_harness.client.get(key=key)).is_equal_to(b"first")

    client_harness.client.delete(key=key)
    client_harness.assert_exists(key=key, expected=False)
    client_harness.assert_missing_get(key=key)

    client_harness.client.set(key=key, value=b"second")
    client_harness.assert_exists(key=key, expected=True)
    assert_that(client_harness.client.get(key=key)).is_equal_to(b"second")


def test_overwrite_chain_keeps_latest_value(client_harness: ClientHarness) -> None:
    key = b"db:chain"
    values = [b"one", b"two", b"three", b"four"]

    for value in values:
        client_harness.client.set(key=key, value=value)
        assert_that(client_harness.client.get(key=key)).is_equal_to(value)

    client_harness.assert_exists(key=key, expected=True)
    client_harness.assert_delete(key=key, expected=True)
    client_harness.assert_missing_get(key=key)


@pytest.mark.parametrize(
    "keys",
    [tuple(f"db:key:{i}".encode("ascii") for i in range(12))],
    ids=["twelve-keys"],
)
def test_write_many_keys_and_delete_subset(
    client_harness: ClientHarness,
    keys: tuple[bytes, ...],
) -> None:
    for index, key in enumerate(keys):
        client_harness.client.set(key=key, value=f"value:{index}".encode("ascii"))

    for index, key in enumerate(keys):
        client_harness.assert_exists(key=key, expected=True)
        assert_that(client_harness.client.get(key=key)).is_equal_to(
            f"value:{index}".encode("ascii")
        )

    for key in keys[::2]:
        client_harness.assert_delete(key=key, expected=True)

    for index, key in enumerate(keys):
        if index % 2 == 0:
            client_harness.assert_exists(key=key, expected=False)
            client_harness.assert_missing_get(key=key)
        else:
            client_harness.assert_exists(key=key, expected=True)
            assert_that(client_harness.client.get(key=key)).is_equal_to(
                f"value:{index}".encode("ascii")
            )
