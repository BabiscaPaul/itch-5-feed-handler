#!/bin/bash
# Build and run in one command
# Usage:
#   ./run.sh                    # build + run with default data file
#   ./run.sh --benchmark        # pass any flags through

set -e

# Build (only recompiles changed files)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 2>&1 | grep -v "^--" || true
cmake --build build 2>&1

DATA="01302019.NASDAQ_ITCH50"

echo "───────────────────────────────"
./build/itch_handler "$DATA" "$@"
