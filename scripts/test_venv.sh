#!/bin/bash

if [ ! -d .test-venv ]; then
    python3 -m venv .test-venv
fi

source .test-venv/bin/activate

python -m pip install --upgrade pip
python -m pip install -r ./test-requirements.txt

pip wheel ./src/sdk/python --no-deps -w dist/
pip install -e ./src/sdk/python
