#!/usr/bin/env python3
"""cegis_adaptive.py — Adaptive CEGIS for Non-Linear Crisis Modeling.

Synthesizes highly modular networks (Islands and Bridges) capable of
surviving a high-velocity Burner (v hops/turn) using an adaptive,
lagging Builder response curve c(t).
"""

import json
import subprocess
import sys
import time

from graph6 import DENDRO_MAX_EDGES, DENDRO_MAX_NODES, to_graph6

GRAPH6_DEPTH_LIMIT = 10
ORACLE_TIMEOUT_SECONDS = 60

try:
    import z3
except ImportError:
    print("Error: z3-solver required.")
    sys.exit(1)


def build_adaptive_model(N, E_target):
    """Z3 model to synthesize a graph on N vertices with >= E edges."""
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

    # Connectedness (No isolated nodes)
    for i in range(N):
        deg_i = z3.Sum([A[i][j] for j in range(N)])
        s.add(deg_i >= 1)

    return s, A


def call_adaptive_oracle(adj_matrix, N, velocity_v, builder_curve):
    """
    Calls the C++ Dendro engine (PSPACE Oracle).
    Extracts the 'Contagion Subgraph' if the Burner overwhelms the Builder.
    """
    if velocity_v != 1:
        raise ValueError("Dendro graph6 oracle currently supports velocity_v=1 only")
    if builder_curve != [1]:
        raise ValueError(
            "Dendro graph6 oracle currently supports builder_curve=[1] only"
        )
    edge_count = sum(adj_matrix[i][j] for i in range(N) for j in range(i + 1, N))
    if not 1 <= N <= DENDRO_MAX_NODES or edge_count > DENDRO_MAX_EDGES:
        raise ValueError(
            "Dendro graph6 oracle supports 1-32 nodes and at most 63 edges"
        )
    if 2 * N - 1 > GRAPH6_DEPTH_LIMIT:
        raise ValueError(
            "Dendro graph6 adaptive oracle has a fixed 10-ply horizon; "
            f"{N} nodes can require {2 * N - 1} plies for a full-game certificate"
        )

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

        if nash >= (N + 1) // 2:
            return nash, attack_edges
        return nash, []

    except subprocess.TimeoutExpired as e:
        print(f"\nOracle Error: Dendro timed out after {e.timeout} seconds")
        print(f"Stdout: {e.stdout}")
        print(f"Stderr: {e.stderr}")
        sys.exit(1)
    except Exception as e:
        print(f"\nOracle Error: {e}")
        if isinstance(e, subprocess.CalledProcessError):
            print(f"Stdout: {e.stdout}")
            print(f"Stderr: {e.stderr}")
        sys.exit(1)


def run_adaptive_cegis(N, E_target, velocity_v, builder_curve):
    print("\n\033[1;35m=== Adaptive CEGIS: Velocity vs. Response ===\033[0m")
    print(f"Nodes: {N} | Target Edges: >= {E_target}")
    print(f"Burner Velocity: v = {velocity_v} hops/turn")
    print(f"Builder Curve c(t): {builder_curve} cuts per turn\n")

    s, A = build_adaptive_model(N, E_target)

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
            print(
                "The Builder's response lag is too severe. No topology can defend this density!"
            )
            return False

        model = s.model()
        adj_matrix = [[model[A[i][j]].as_long() for j in range(N)] for i in range(N)]
        print(f"Done ({time.time() - start_z3:.2f}s)")

        print("  -> Passing to Adaptive Oracle... ", end="", flush=True)

        actual_damage, attack_edges = call_adaptive_oracle(
            adj_matrix, N, velocity_v, builder_curve
        )

        if actual_damage < (N + 1) // 2:
            print("\033[1;32mSUCCESS!\033[0m")
            print(
                "\n\033[1;32m✓ PROOF COMPLETE: Synthesized Modularity (Islands & Bridges).\033[0m"
            )
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
            print(
                f"  -> Burner's v={velocity_v} burst exploited a high-bandwidth corridor!"
            )
            print(f"  -> Flank extracted: {attack_edges}")

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
    # Test Case: N=5, E=8 (largest N supported by the fixed 10-ply Dendro depth)
    # Burner: v=3 burst. Builder: lagging response [1, 5, 10]
    run_adaptive_cegis(N=5, E_target=8, velocity_v=1, builder_curve=[1])
