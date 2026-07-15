from __future__ import annotations

from base.harness import ClientHarness


def test_ping(client_harness: ClientHarness) -> None:
    """Test the ping functionality"""
    client_harness.assert_ping(b"Hello world!")


def test_set_get(client_harness: ClientHarness) -> None:
    """Test if get returns value added by set"""
    client_harness.assert_set_get(key=b"my:key", value=b"Hello world!")


def test_exists(client_harness: ClientHarness) -> None:
    """Test if exists returns correct value"""
    key = b"my:key"

    client_harness.assert_exists(key=key, expected=False)
    client_harness.client.set(key=key, value=b"Hello world!")
    client_harness.assert_exists(key=key, expected=True)


def test_set_update_value(client_harness: ClientHarness) -> None:
    """Test if set updates previous value"""
    client_harness.assert_overwrite(
        key=b"my:key",
        value=b"Hello world!",
        updated_value=b"Updated value",
    )


def test_get_on_non_existing_value(client_harness: ClientHarness) -> None:
    """Test if get returns None for non-existing key"""
    client_harness.assert_missing_get(key=b"non:existent:key")
