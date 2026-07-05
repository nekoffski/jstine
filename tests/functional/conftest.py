from __future__ import annotations

import pytest

from base.system import System
from base.artifacts import TestArtifacts


System.os()


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
