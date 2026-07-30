#!/usr/bin/env bash

source .test-venv/bin/activate
pytest tests/functional/test_*.py --verbose "$@"
