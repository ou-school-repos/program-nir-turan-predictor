#!/usr/bin/env bash
# sweep_bipartite.sh — Exhaustive bipartite Leontovich sweep
#
# Generates all connected bipartite graphs on m vertices (via genbg),
# deduplicates across partitions (via shortg), and pipes through
# leontovich_fast.  Counts are validated against OEIS A005142.
#
# Usage:  ./scripts/sweep_bipartite.sh [M_MAX]
#         Default M_MAX=15
#
# Output: docs/runs/bipartite_sweep.log  (append-only audit log)

set -euo pipefail

M_MAX="${1:-15}"
LOGFILE="docs/runs/bipartite_sweep.log"
FILTER="./leontovich_fast"

# OEIS A005142: number of connected bipartite graphs on n nodes
# (parts interchangeable), n=0..15
A005142=(1 1 1 1 3 5 17 44 182 730 4032 25598 212780 2241730 31193324 575252112)

mkdir -p docs/runs

{
  echo "=== Bipartite Leontovich sweep ==="
  echo "Started: $(date -Iseconds)"
  echo "M_MAX=$M_MAX"
  echo ""
} | tee -a "$LOGFILE"

grand_total=0
grand_hits=0

for m in $(seq 1 "$M_MAX"); do
  t0=$(date +%s)

  echo "--- m=$m ---" | tee -a "$LOGFILE"

  # Generate all partitions, dedup with shortg, pipe to filter
  # Capture all filter output (stderr) to both log and variable
  json=$(
    (for n1 in $(seq 1 $((m / 2))); do
      genbg -c "$n1" $((m - n1)) -q
    done) |
      shortg -q 2>/dev/null |
      "$FILTER" 2>&1 >/dev/null |
      tee -a "$LOGFILE"
  )

  t1=$(date +%s)
  elapsed=$((t1 - t0))

  total=$(echo "$json" | grep -oP '"total":\K\d+' || echo 0)
  hits=$(echo "$json" | grep -oP '"hits":\K\d+' || echo 0)

  # Validate against A005142 if we have the value
  expected="${A005142[$m]:-?}"
  if [ "$expected" != "?" ] && [ "$total" -eq "$expected" ]; then
    status="✓ A005142"
  elif [ "$expected" != "?" ]; then
    status="✗ MISMATCH (expected $expected)"
  else
    status="? (no A005142 ref)"
  fi

  grand_total=$((grand_total + total))
  grand_hits=$((grand_hits + hits))

  line="m=$m | $total graphs | $hits hits | ${elapsed}s | $status"
  echo "$line" | tee -a "$LOGFILE"
  echo "" | tee -a "$LOGFILE"
done

{
  echo "Grand total: $grand_total graphs, $grand_hits hits"
  echo "Finished: $(date -Iseconds)"
  echo '{"event":"bipartite_sweep_done","m_max":'"$M_MAX"',"total":'"$grand_total"',"hits":'"$grand_hits"'}'
} | tee -a "$LOGFILE"
