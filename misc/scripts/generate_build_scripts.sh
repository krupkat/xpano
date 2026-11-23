#!/usr/bin/env bash

set -eu

if [ -z "${PYTHON-}" ]; then
    PYTHON=python
    command -v python3 >/dev/null && PYTHON=python3
fi

"$PYTHON" ./misc/scripts/python/translate_github_action.py .github/workflows/test.yml --output_dir misc/build
