#!/bin/bash
# Exhaustive H-coloring sweep: even + odd paths
# Tests Problem 6.3 (Galvin-McMillon-Nir 2026): do odd paths minimize?
# Results logged to docs/runs/sequence.jsonl
#
# Usage:
#   bash scripts/sweep_hcolor.sh          # default: n=0..20
#   bash scripts/sweep_hcolor.sh 21 23    # custom:  n=21..23

set -e
cd "$(dirname "$0")/.."

LO=${1:-0}
HI=${2:-20}

echo "=== H-Coloring Conjecture Sweep (n=$LO..$HI) ==="
echo ""

# # ── Phase 1: Even paths (known result: path minimizes) ──
# echo "── Phase 1: Even paths P2,P4,P6 (known: path minimizes) ──"
# for h in P2 P4 P6; do
#   for n in $(seq $LO $HI); do
#     echo "[$h n=$n]"
#     ./synthesizer $n --hcolor $h 2>&1
#     echo ""
#   done
#   echo "--- $h complete ---"
# done

# echo ""

# ── Phase 2: Odd paths (Problem 6.3 — OPEN) ──
echo "── Phase 2: Odd paths P3,P5,P7,P9 (Problem 6.3: open) ──"
for h in P3 P5 P7 P9; do
  for n in $(seq $LO $HI); do
    echo "[$h n=$n]"
    ./synthesizer $n --hcolor $h 2>&1
    echo ""
  done
  echo "--- $h complete ---"
done

echo ""
echo "=== Sweep complete. Results in docs/runs/sequence.jsonl ==="
