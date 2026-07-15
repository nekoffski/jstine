from __future__ import annotations

from base.context import Context
from base.harness import ClientHarness


def test_default_expiration_policy(
    ctx: Context,
    client_harness: ClientHarness,
) -> None:
    """ Test that value is reaped after default expiration period is over """
    key = b"test:key"
    default_expiration = (
        ctx.server_config().storage.default_expiration_seconds
    )

    client_harness.assert_exists(key=key, expected=False)
    client_harness.assert_set(key, value="some:test:value")
    client_harness.assert_exists(key=key, expected=True)
    ctx.sleep(default_expiration)
    client_harness.assert_exists(key=key, expected=False)
