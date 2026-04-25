#!/usr/bin/env python3
"""Analyze caterpillar Nash equilibrium sweep data.

Reads docs/runs/caterpillar_nash.csv and characterizes growth rates.
Telecom interpretation: spine = fiber backbone, legs = access nodes.
Drone warfare: spine = patrol corridor, legs = observation posts.
"""

import csv
from collections import defaultdict

CSV = "docs/runs/caterpillar_nash.csv"


def main():
    data = defaultdict(list)  # k -> [(spine, N, nash)]
    with open(CSV) as f:
        reader = csv.DictReader(f)
        for row in reader:
            k = int(row["legs"])
            s = int(row["spine"])
            n = int(row["N"])
            nash = int(row["nash"])
            data[k].append((s, n, nash))

    print("=" * 70)
    print("CATERPILLAR TREE ADVERSARIAL ANALYSIS")
    print("Maker-Breaker Burning Game: Nash Equilibrium Builder Limits")
    print("=" * 70)

    print("\n--- Raw Data ---")
    print(f"{'k':>3} {'spine':>6} {'N':>5} {'nash':>5} {'nash/N':>8}")
    print("-" * 30)
    for k in sorted(data.keys()):
        for s, n, nash in data[k]:
            print(f"{k:>3} {s:>6} {n:>5} {nash:>5} {nash/n:>8.3f}")
        print()

    print("--- Constant Builder Limit Conjecture ---")
    print()
    print("  Conjecture: For caterpillar C(S,K) with S >= K+2,")
    print("  the Nash value of the adversarial burning game is")
    print()
    print("      nash(C(S,K)) = K + 2")
    print()
    print("  independent of spine length S.")
    print()

    # Verify conjecture
    holds = True
    for k in sorted(data.keys()):
        expected = k + 2
        violations = [
            (s, n, nash) for s, n, nash in data[k] if nash != expected and s >= k + 2
        ]
        if violations:
            holds = False
            print(f"  [FAIL] k={k}: expected {expected}, got:")
            for s, n, nash in violations:
                print(f"    S={s}, N={n}: nash={nash}")
        else:
            stable = [nash for s, _, nash in data[k] if s >= k + 2]
            if stable:
                print(
                    f"  [PASS] k={k}: nash={stable[0]} = {k}+2 ✓"
                    f" (verified for S={k + 2}"
                    f"..{max(s for s, _, _ in data[k])})"
                )

    print()
    if holds:
        print("  ★ CONJECTURE HOLDS for all tested values.")
    else:
        print("  ✗ Conjecture violated — see failures above.")

    print()
    print("--- Telecom / Drone Warfare Interpretation ---")
    print()
    print("  In a backbone network with K access points per hub:")
    print("  An adversary cutting links can always limit infection")
    print("  to at most K+2 nodes, regardless of backbone length.")
    print()
    print("  Implication: Network resilience scales with hub degree,")
    print("  not with backbone length. Adding more hubs along the")
    print("  backbone does NOT increase vulnerability.")
    print()
    print("  For drone patrol corridors with K observation posts:")
    print("  A saboteur severing communication links can contain")
    print("  any intrusion to K+2 sectors regardless of corridor length.")


if __name__ == "__main__":
    main()
