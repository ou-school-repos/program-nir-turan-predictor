#!/usr/bin/env bash
# Sweep adversarial Maker-Breaker game on generalized caterpillar trees.
# Caterpillar C(S,K): spine length S, K legs per spine node, N = S*(K+1) nodes.
# Output: CSV to stdout, telemetry to stderr.
set -euo pipefail

DENDRO="./dendro"
CSV_OUT="docs/runs/caterpillar_nash.csv"

echo "spine,legs,N,depth,nash,states,time_ms" | tee "$CSV_OUT"

for k in 0 1 2 3 4; do
  for s in $(seq 3 12); do
    n=$((s * (k + 1)))
    if [ "$n" -gt 32 ]; then continue; fi # skip infeasible sizes

    if [ "$k" -eq 0 ]; then
      preset="path${n}"
    else
      preset="cat${s}x${k}"
      if [ "$k" -eq 1 ]; then preset="cat${s}"; fi
    fi

    # Run adversarial and capture output
    output=$($DENDRO adversarial /dev/null "$preset" 2>&1)
    nash=$(echo "$output" | grep -oP 'destruction to \K\d+')
    states=$(echo "$output" | grep -oP '\K\d+(?= states)')
    time_ms=$(echo "$output" | grep -oP '\K\d+(?= ms)')

    echo "${s},${k},${n},_,${nash},${states},${time_ms}" | tee -a "$CSV_OUT"
    >&2 echo "  [done] ${preset}: nash=${nash}, ${states} states, ${time_ms}ms"
  done
done

>&2 echo ""
>&2 echo "Results written to ${CSV_OUT}"
