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

run_case() {
  local cfg="$1"
  local start end
  start="$(date +%s%N)"
  {
    printf 'write beta payload\n'
    for i in $(seq 1 200); do
      printf 'read beta\n'
    done
    printf 'quit\n'
  } | ./bin/cn --config "$cfg" >/tmp/dist-td-bench.out
  end="$(date +%s%N)"
  echo $(((end - start) / 1000))
}

ON_US="$(run_case build/config/cn.rdma.conf)"
OFF_US="$(run_case build/config/cn.rdma.off.conf)"

echo "cache_on_total_us=$ON_US"
echo "cache_off_total_us=$OFF_US"
