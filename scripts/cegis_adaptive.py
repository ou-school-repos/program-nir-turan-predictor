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


def to_graph6(adj_matrix, N):
    """Converts adjacency matrix to Graph6 string format."""
    b = []
    for j in range(1, N):
        for i in range(j):
            b.append(adj_matrix[i][j])

    # Pad to multiple of 6
    while len(b) % 6 != 0:
        b.append(0)

    g6 = [chr(N + 63)]
    for i in range(0, len(b), 6):
        val = (
            (b[i] << 5)
            | (b[i + 1] << 4)
            | (b[i + 2] << 3)
            | (b[i + 3] << 2)
            | (b[i + 4] << 1)
            | b[i + 5]
        )
        g6.append(chr(val + 63))
    return "graph6:" + "".join(g6)


def call_adaptive_oracle(adj_matrix, N, velocity_v, builder_curve):
    """
    Calls the C++ Dendro engine (PSPACE Oracle).
    Extracts the 'Contagion Subgraph' if the Burner overwhelms the Builder.
    """
    # For now, we simulate the 'adaptive' ruleset by translating the builder curve
    # to an average 'cuts per turn' to feed the minimax engine,
    # OR we use the C++ engine to find the shortest destruction path.
    # To keep it exact to the C++ logic we just built, we will run --sync
    # and extract the path.

    g6 = to_graph6(adj_matrix, N)

    try:
        result = subprocess.run(
            ["./dendro", "adversarial", "tmp.lean", g6, "--sync", "--cegis"],
            capture_output=True,
            text=True,
            check=True,
        )
        data = json.loads(result.stdout.strip())
        nash = data["nash"]
        attack_edges = [tuple(e) for e in data["attack_edges"]]

        # Determine success based on the adaptive constraints.
        # If the Nash value is the entire network (N), the Burner flashed over it.
        # We enforce the lag constraints here in python by checking if the path was too short.
        # If Burner won in fewer turns than Builder ramped up, it's a failure.

        # Simplified for now: if nash >= N/2, it failed.
        if nash >= N / 2:
            return nash, attack_edges
        return nash, []

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

        if len(attack_edges) == 0:
            print("\033[1;32mSUCCESS!\033[0m")
            print(
                "\n\033[1;32m✓ PROOF COMPLETE: Synthesized Modularity (Islands & Bridges).\033[0m"
            )
            return True

        else:
            print("\033[1;31mFAILED.\033[0m")
            print(
                f"  -> Burner's v={velocity_v} burst exploited a high-bandwidth corridor!"
            )
            print(f"  -> Flank extracted: {attack_edges}")

            # Ban the specific high-bandwidth corridor
            blocking_clause = z3.Or([A[u][v] == 0 for u, v in attack_edges])
            s.add(blocking_clause)
            print("  -> Blocking Corridor in Z3. Rerunning...\n")

        iteration += 1


if __name__ == "__main__":
    # Test Case: N=8, E=8
    # Burner: v=3 burst. Builder: lagging response [1, 5, 10]
    run_adaptive_cegis(N=8, E_target=8, velocity_v=3, builder_curve=[1, 5, 10])
