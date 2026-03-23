#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

make >/dev/null

cleanup() {
  pkill -f "bin/mn --config build/config/mn.rdma.conf" >/dev/null 2>&1 || true
}
trap cleanup EXIT

./bin/mn --config build/config/mn.rdma.conf >/tmp/dist-td-mn.log 2>&1 &
sleep 1

OUTPUT="$(printf 'write alpha hello\nread alpha\nupdate alpha world\nread alpha\ndelete alpha\nread alpha\nquit\n' | ./bin/cn --config build/config/cn.rdma.conf)"
printf '%s\n' "$OUTPUT"

grep -q "ok alpha" <<<"$OUTPUT"
grep -q "value alpha hello" <<<"$OUTPUT"
grep -q "value alpha world" <<<"$OUTPUT"
grep -q "not_found alpha" <<<"$OUTPUT"
