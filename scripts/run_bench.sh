#!/bin/bash

source .test-venv/bin/activate
pytest tests/functional/bench_*.py --verbose "$@"
