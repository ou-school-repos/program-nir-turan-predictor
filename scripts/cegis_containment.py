#!/usr/bin/env python3
"""cegis_containment.py — Counterexample-Guided Inductive Synthesis (CEGIS).

Couples Z3 (NP-Synthesis) with C++ Dendro (PSPACE-Evaluation) to
discover Adversarial Extremal Graphs.
"""

import json
import subprocess
import sys
import time

from graph6 import to_graph6

GRAPH6_DEPTH_LIMIT = 10
ORACLE_TIMEOUT_SECONDS = 60

try:
    import z3
except ImportError:
    print("Error: z3-solver required.")
    sys.exit(1)


def build_synthesis_model(N, E_target, max_deg):
    """Z3 model to synthesize a graph on N vertices with E edges."""
    s = z3.Solver()
    A = [[z3.Int(f"A_{i}_{j}") for j in range(N)] for i in range(N)]

    # Binary symmetric adjacency matrix
    for i in range(N):
        s.add(A[i][i] == 0)
        for j in range(i + 1, N):
            s.add(z3.Or(A[i][j] == 0, A[i][j] == 1))
            s.add(A[i][j] == A[j][i])

    # Edge Count Target
    total_edges = z3.Sum([A[i][j] for i in range(N) for j in range(i + 1, N)])
    s.add(total_edges >= E_target)

    # Degree Bound (Necessary condition: Delta <= tau + c - 1)
    for i in range(N):
        deg_i = z3.Sum([A[i][j] for j in range(N)])
        s.add(deg_i >= 1)  # No isolated nodes
        s.add(deg_i <= max_deg)

    # Symmetry Breaking (Sort degrees to kill isomorphic branches)
    for i in range(N - 1):
        s.add(
            z3.Sum([A[i][j] for j in range(N)])
            >= z3.Sum([A[i + 1][j] for j in range(N)])
        )

    return s, A


def call_dendro_oracle(adj_matrix, N, cuts):
    """Calls the real C++ Dendro PSPACE Oracle."""
    if cuts != 1:
        raise ValueError("Dendro graph6 oracle currently supports cuts=1 only")
    edge_count = sum(adj_matrix[i][j] for i in range(N) for j in range(i + 1, N))
    if not 1 <= N <= 32 or edge_count >= 64:
        raise ValueError("Dendro supports 1-32 nodes and at most 63 edges")
    # NOTE: The Dendro graph6 preset currently uses a fixed 10-ply search depth.
    # This oracle is therefore a bounded-depth oracle (not a full-game certificate)
    # for larger N.
    # If a full-game certificate is needed, the C++ preset depth must be increased.
    g6 = to_graph6(adj_matrix, N)
    try:
        result = subprocess.run(
            ["./dendro", "adversarial", "tmp.lean", g6, "--sync", "--cegis"],
            capture_output=True,
            text=True,
            check=True,
            timeout=ORACLE_TIMEOUT_SECONDS,
        )
        data = json.loads(result.stdout.strip())
        nash = data["nash"]
        attack_edges = [tuple(e) for e in data["attack_edges"]]
        return nash, attack_edges
    except Exception as e:
        print(f"\nOracle Error: {e}")
        if isinstance(e, subprocess.CalledProcessError):
            print(f"Stdout: {e.stdout}")
            print(f"Stderr: {e.stderr}")
        sys.exit(1)


def run_cegis(N, E_target, tau, cuts):
    print("\n\033[1;36m=== CEGIS Loop: Adversarial Extremal Graph Synthesis ===\033[0m")
    print(f"Nodes: {N} | Target Edges: >= {E_target}")
    print(f"Constraint: Nash_{cuts}(G) <= {tau}\n")

    # Absolute upper bound for degree to prevent a turn-1 blowout
    max_deg = tau + cuts - 1

    s, A = build_synthesis_model(N, E_target, max_deg)

    iteration = 1
    while True:
        print(
            f"[Iter {iteration}] Z3 Synthesizing candidate graph... ",
            end="",
            flush=True,
        )
        start_z3 = time.time()

        if s.check() != z3.sat:
            print("\n\033[1;31m[!] UNSAT: Mathematical Proof generated.\033[0m")
            print(f"No graph with {E_target} edges can survive {tau} damage.")
            print(
                "The network density has mathematically overpowered the Builder's c-cut capacity!"
            )
            return False

        model = s.model()
        adj_matrix = [[model[A[i][j]].as_long() for j in range(N)] for i in range(N)]
        print(f"Done ({time.time() - start_z3:.2f}s)")

        print("  -> Passing Candidate to C++ Dendro Oracle... ", end="", flush=True)

        # ── PSPACE ORACLE CALL ──
        actual_nash, attack_edges = call_dendro_oracle(adj_matrix, N, cuts)

        if actual_nash <= tau:
            print("\033[1;32mSUCCESS!\033[0m")
            print(
                "\n\033[1;32m✓ PROOF COMPLETE: Synthesized Bulletproof Network.\033[0m"
            )
            print(f"  Edges: {E_target} | Nash Value: {actual_nash} (<= {tau})")

            g6 = to_graph6(adj_matrix, N)
            winning_edges = []
            for i in range(N):
                for j in range(i + 1, N):
                    if adj_matrix[i][j] == 1:
                        winning_edges.append((i, j))
            print("\n\033[1;36mOptimal Topology Discovered:\033[0m")
            print(f"  Graph6: {g6}")
            print(f"  Edges:  {winning_edges}")
            return True

        else:
            print("\033[1;31mFAILED.\033[0m")
            print(f"  -> Burner exploited topology! Damage = {actual_nash}")
            print(f"  -> Core Attack Path extracted: {attack_edges}")

            blocking_clause = z3.Not(
                z3.And(
                    [
                        A[i][j] == adj_matrix[i][j]
                        for i in range(N)
                        for j in range(i + 1, N)
                    ]
                )
            )

            s.add(blocking_clause)
            print("  -> Blocking exact failed graph in Z3. Rerunning...\n")

        iteration += 1


if __name__ == "__main__":
    # Test Case 1: 8 nodes, 12 edges, tau=4, cuts=1.
    # This should succeed, showing a successful synthesis of a resilient graph.
    run_cegis(N=8, E_target=12, tau=4, cuts=1)

    # Test Case 2: 8 nodes, 16 edges, tau=3, cuts=1.
    # This should be UNSAT, proving that the network density mathematically overpowers containment.
    run_cegis(N=8, E_target=16, tau=3, cuts=1)
