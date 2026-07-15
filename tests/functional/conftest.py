from __future__ import annotations

from collections.abc import AsyncIterator, Iterator

import pytest
import pytest_asyncio

from base.artifacts import TestArtifacts
from base.context import Context
from base.config import Config, ServerRuntimeConfig
from base.harness import AsyncClientHarness, ClientHarness
from base.server import Server
from base.system import System


System.os()


@pytest.fixture
def ctx(request: pytest.FixtureRequest) -> Context:
    test_config = Config.load()
    runtime_config = ServerRuntimeConfig.load(test_config.server.config_path)
    artifacts = TestArtifacts.for_nodeid(request.node.nodeid)
    artifacts.clear()
    request.node._functional_artifacts = artifacts
    return Context(
        test_config=test_config,
        runtime_config=runtime_config,
        artifacts=artifacts,
    )


@pytest.fixture(autouse=True)
def server(ctx: Context) -> Iterator[Server]:
    instance = Server(
        ctx.config().server.binary,
        port=ctx.config().server.port,
        config_path=ctx.config().server.config_path,
        artifacts=ctx.artifacts,
    )
    ctx.server = instance
    instance.start()
    try:
        yield instance
    finally:
        instance.stop()
        ctx.server = None


@pytest.fixture
def client_harness(ctx: Context) -> Iterator[ClientHarness]:
    harness = ClientHarness(ctx)
    try:
        yield harness
    finally:
        harness.close()


@pytest_asyncio.fixture
async def async_client_harness(ctx: Context) -> AsyncIterator[AsyncClientHarness]:
    harness = await AsyncClientHarness.create(ctx)
    try:
        yield harness
    finally:
        await harness.close()


def _artifacts_for(item) -> TestArtifacts:
    artifacts = getattr(item, "_functional_artifacts", None)
    if artifacts is None:
        artifacts = TestArtifacts.for_nodeid(item.nodeid)
        item._functional_artifacts = artifacts
    return artifacts


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item, call):
    report = yield
    rep = report.get_result()
    artifacts = _artifacts_for(item)

    chunks = [f"[{rep.when}] {rep.outcome}\n"]

    if rep.capstdout:
        chunks.append("stdout:\n")
        chunks.append(rep.capstdout)
        if not rep.capstdout.endswith("\n"):
            chunks.append("\n")

    if rep.capstderr:
        chunks.append("stderr:\n")
        chunks.append(rep.capstderr)
        if not rep.capstderr.endswith("\n"):
            chunks.append("\n")

    if rep.caplog:
        chunks.append("log:\n")
        chunks.append(rep.caplog)
        if not rep.caplog.endswith("\n"):
            chunks.append("\n")

    if rep.failed and rep.longreprtext:
        chunks.append("failure:\n")
        chunks.append(rep.longreprtext)
        if not rep.longreprtext.endswith("\n"):
            chunks.append("\n")

    artifacts.append(artifacts.pytest_log, "".join(chunks))
