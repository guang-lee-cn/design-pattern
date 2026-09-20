#!/bin/sh
# Remove all build output directories (Release: build/, Debug: build-debug/).
# POSIX sh: "./clean.sh", "sh clean.sh", and "bash clean.sh" are all equivalent.
set -eu
cd "$(dirname "$0")"

rm -rf build build-debug
echo "cleaned build artifacts: build/ build-debug/"
