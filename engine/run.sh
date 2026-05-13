#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <itch-file> [symbols-file]"
    echo "  e.g. $0 feeds/01302019.NASDAQ_ITCH50"
    exit 1
fi

ITCH="$1"
SYMS="${2:-symbols.txt}"

# Derive run-id from filename: 01302019.NASDAQ_ITCH50 -> 2019-01-30
base=$(basename "$ITCH" .NASDAQ_ITCH50)
RUN_ID="${base:4:4}-${base:0:2}-${base:2:2}"
OUT="../data/runs/$RUN_ID"

echo "Run ID: $RUN_ID"
echo "Output: $OUT"
echo

exec ./build/itch_handler "$ITCH" "$SYMS" "$OUT"
