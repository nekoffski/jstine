#!/bin/bash

set -e 

make fmt-check
make build-debug
make test

./scripts/run_functionals.sh

