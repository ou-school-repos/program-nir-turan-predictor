#!/bin/bash
# Sweep N=3..MAX to extract the Δ≤3 and Δ≤4 maximum IS sequences
# Usage: ./scripts/sweep_d3.sh [MAX_N]
#
# Output: docs/runs/sequence_d3.csv

set -e
cd "$(dirname "$0")/.."

MAX=${1:-23}
OUTFILE="docs/runs/sequence_d3.csv"

echo "N,max_d3,max_d4,max_any,path_score" >"$OUTFILE"

# N=1,2 are trivial
echo "1,1,1,1,2" >>"$OUTFILE"
echo "2,2,2,2,3" >>"$OUTFILE"

for N in $(seq 3 "$MAX"); do
  echo "=== N=$N ===" >&2
  JSON=$(./synthesizer "$N" --top 1 2>>"docs/runs/sweep.log")

  # Extract scores from JSON using python
  ROW=$(echo "$JSON" | python3 -c "
import json, sys
d = json.load(sys.stdin)
ps = d['path_score']
tk = d.get('top_k', [])
d3 = d4 = dany = 0
for t in tk:
    c = t.get('constraint', '')
    if 'Delta <= 3' in c: d3 = t['score']
    elif 'Delta <= 4' in c: d4 = t['score']
    elif 'Unconstrained' in c: dany = t['score']
print(f'{d[\"n\"]},{d3},{d4},{dany},{ps}')
")
  echo "$ROW" >>"$OUTFILE"
  echo "$ROW" >&2
done

echo "" >&2
echo "=== Sequence complete ===" >&2
echo "Δ≤3 sequence:" >&2
cut -d, -f2 "$OUTFILE" | tail -n +2 | tr '\n' ',' >&2
echo "" >&2
echo "Results saved to $OUTFILE" >&2
