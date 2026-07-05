from __future__ import annotations

import pytest

from base.artifacts import TestArtifacts
from base.system import System


System.os()


def pytest_addoption(parser):
    parser.addoption(
        "--run-benchmarks",
        action="store_true",
        default=False,
        help="run tests marked with benchmark",
    )


def pytest_configure(config):
    config.addinivalue_line(
        "markers",
        "benchmark: tests that are skipped unless --run-benchmarks is set",
    )


def _artifacts_for(item) -> TestArtifacts:
    artifacts = getattr(item, "_functional_artifacts", None)
    if artifacts is None:
        artifacts = TestArtifacts.for_nodeid(item.nodeid)
        item._functional_artifacts = artifacts
    return artifacts


def pytest_collection_modifyitems(config, items):
    if config.getoption("--run-benchmarks"):
        return

    skip_benchmark = pytest.mark.skip(
        reason="benchmark tests are skipped by default; pass --run-benchmarks"
    )
    for item in items:
        if "benchmark" in item.keywords:
            item.add_marker(skip_benchmark)


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
