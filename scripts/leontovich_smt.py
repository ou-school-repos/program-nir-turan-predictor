#!/usr/bin/env python3
"""leontovich_smt.py — SMT-based search for Leontovich graphs.

Uses the Z3 theorem prover to find target graphs H of order m for which
the path P_n is not the tree-homomorphism minimizer at threshold n.
"""

import argparse
import sys
import time

try:
    import z3
except ImportError:
    print("Error: The 'z3-solver' package is required to run this script.")
    print("Please install it via: pip install z3-solver")
    sys.exit(1)


def to_graph6(A):
    """Encode an adjacency matrix into a graph6 string."""
    m = len(A)
    if m > 62:
        raise ValueError(
            "graph6 encoding for m > 62 is more complex and not implemented here."
        )
    g6 = chr(m + 63)
    bits = []
    for col in range(1, m):
        for row in range(col):
            bits.append(A[row][col])
    # Pad bits with 0s to multiple of 6
    while len(bits) % 6 != 0:
        bits.append(0)
    # Convert to characters
    for i in range(0, len(bits), 6):
        val = 0
        for b in range(6):
            val = (val << 1) | bits[i + b]
        g6 += chr(val + 63)
    return g6


def build_leontovich_solver_partition(
    m1, m2, n, d=2, graph_type="bipartite", max_deg=None
):
    """Build a Z3 Solver for finding a bipartite target graph H of partition (m1, m2)
    using a highly optimized block-matrix formulation.
    """
    m = m1 + m2
    s = z3.Solver()

    # 1. Block matrix variables: B[i][j] in {0, 1} for i in range(m1), j in range(m2)
    B = [[z3.Int(f"B_{i}_{j}") for j in range(m2)] for i in range(m1)]
    for i in range(m1):
        for j in range(m2):
            s.add(z3.Or(B[i][j] == 0, B[i][j] == 1))

    # 2. Basic degree and connectivity constraints
    row_degs = [z3.Sum([B[i][j] for j in range(m2)]) for i in range(m1)]
    col_degs = [z3.Sum([B[i][j] for i in range(m1)]) for j in range(m2)]

    for i in range(m1):
        s.add(row_degs[i] >= 1)
        if max_deg is not None:
            s.add(row_degs[i] <= max_deg)
    for j in range(m2):
        s.add(col_degs[j] >= 1)
        if max_deg is not None:
            s.add(col_degs[j] <= max_deg)

    # If it is a tree: number of edges is m - 1 = m1 + m2 - 1
    if graph_type == "tree":
        num_edges = z3.Sum([B[i][j] for i in range(m1) for j in range(m2)])
        s.add(num_edges == m - 1)

    # 3. Connectivity Constraints (Distance levels from Row 0)
    dist_row = [z3.Int(f"dist_row_{i}") for i in range(m1)]
    dist_col = [z3.Int(f"dist_col_{j}") for j in range(m2)]

    s.add(dist_row[0] == 0)
    for i in range(1, m1):
        s.add(dist_row[i] >= 2)
        s.add(dist_row[i] <= m - 1)
        # Connected to at least one column with dist_col[j] == dist_row[i] - 1
        s.add(
            z3.Or(
                [
                    z3.And(B[i][j] == 1, dist_col[j] == dist_row[i] - 1)
                    for j in range(m2)
                ]
            )
        )

    for j in range(m2):
        s.add(dist_col[j] >= 1)
        s.add(dist_col[j] <= m - 1)
        # Connected to at least one row with dist_row[i] == dist_col[j] - 1
        s.add(
            z3.Or(
                [
                    z3.And(B[i][j] == 1, dist_row[i] == dist_col[j] - 1)
                    for i in range(m1)
                ]
            )
        )

    # 4. Symmetry Breaking: Lexicographical and Degree Ordering
    for i in range(m1 - 1):
        s.add(row_degs[i] >= row_degs[i + 1])
    for j in range(m2 - 1):
        s.add(col_degs[j] >= col_degs[j + 1])

    # Lexicographical ordering on rows of B
    def lex_gte(list1, list2):
        conds = []
        for k in range(len(list1)):
            eq = z3.And([list1[x] == list2[x] for x in range(k)])
            conds.append(z3.And(eq, list1[k] > list2[k]))
        conds.append(z3.And([list1[x] == list2[x] for x in range(len(list1))]))
        return z3.Or(conds)

    for i in range(m1 - 1):
        s.add(lex_gte(B[i], B[i + 1]))

    for j in range(m2 - 1):
        col_j = [B[i][j] for i in range(m1)]
        col_j1 = [B[i][j + 1] for i in range(m1)]
        s.add(lex_gte(col_j, col_j1))

    # 5. Walk Vectors computation (Dynamic Programming representation)
    w = [[z3.Int(f"w_{k}_{i}") for i in range(m)] for k in range(n)]

    # w_0 = all-ones vector
    for i in range(m):
        s.add(w[0][i] == 1)

    # w_k = A * w_{k-1}
    for k in range(1, n):
        # Rows: i < m1
        for i in range(m1):
            s.add(w[k][i] == z3.Sum([B[i][j] * w[k - 1][m1 + j] for j in range(m2)]))
        # Columns: j < m2 (i = m1 + j)
        for j in range(m2):
            s.add(w[k][m1 + j] == z3.Sum([B[i][j] * w[k - 1][i] for i in range(m1)]))

    # 6. Homomorphism Counts
    homP = z3.Int("homP")
    s.add(homP == z3.Sum([w[n - 1][i] for i in range(m)]))

    b = [z3.Int(f"b_{i}") for i in range(m)]
    for i in range(m):
        s.add(b[i] == w[1][i] * w[d][i])

    homE = z3.Int("homE")
    s.add(homE == z3.Sum([w[n - d - 2][i] * b[i] for i in range(m)]))

    s.add(homP > 0)
    s.add(homE > 0)
    s.add(homE < homP)

    return s, B, homP, homE


def build_leontovich_solver_general(m, n, d=2, max_deg=None):
    """Build a general-graph solver (unpartitioned, potentially non-bipartite)."""
    s = z3.Solver()

    # 1. Adjacency Matrix Variables: A[i][j] in {0, 1}
    A = [[z3.Int(f"A_{i}_{j}") for j in range(m)] for i in range(m)]
    for i in range(m):
        s.add(A[i][i] == 0)
        for j in range(i + 1, m):
            s.add(A[i][j] == A[j][i])
            s.add(z3.Or(A[i][j] == 0, A[i][j] == 1))

    # 2. Graph Connectivity Constraints
    dist = [z3.Int(f"dist_{i}") for i in range(m)]
    s.add(dist[0] == 0)
    for i in range(1, m):
        s.add(dist[i] >= 1)
        s.add(dist[i] <= m - 1)
        s.add(z3.Or([z3.And(A[i][j] == 1, dist[j] == dist[i] - 1) for j in range(m)]))

    # 3. Degree Bounds
    degs = [z3.Sum([A[i][j] for j in range(m)]) for i in range(m)]
    for i in range(m):
        s.add(degs[i] >= 1)
        if max_deg is not None:
            s.add(degs[i] <= max_deg)

    # 4. Symmetry Breaking (Sort degrees)
    for i in range(m - 1):
        s.add(degs[i] >= degs[i + 1])

    # 5. Walk Vectors
    w = [[z3.Int(f"w_{k}_{i}") for i in range(m)] for k in range(n)]
    for i in range(m):
        s.add(w[0][i] == 1)

    for k in range(1, n):
        for i in range(m):
            s.add(w[k][i] == z3.Sum([A[i][j] * w[k - 1][j] for j in range(m)]))

    # 6. Homomorphism Counts
    homP = z3.Int("homP")
    s.add(homP == z3.Sum([w[n - 1][i] for i in range(m)]))

    b = [z3.Int(f"b_{i}") for i in range(m)]
    for i in range(m):
        s.add(b[i] == w[1][i] * w[d][i])

    homE = z3.Int("homE")
    s.add(homE == z3.Sum([w[n - d - 2][i] * b[i] for i in range(m)]))

    s.add(homP > 0)
    s.add(homE > 0)
    s.add(homE < homP)

    return s, A, homP, homE


def run_smt_search(
    m, n, d=2, graph_type="bipartite", m1=None, m2=None, max_deg=None, timeout_ms=None
):
    """Run SMT search and return results."""
    print(
        f"\n\033[1;36mInitializing Z3 SMT search for target H of order m={m}...\033[0m"
    )
    print(f"  Threshold n:      {n}")
    print(f"  Candidate:        E_n^({d})")
    print(f"  Graph Class:      {graph_type}")
    if max_deg:
        print(f"  Max Degree:       {max_deg}")
    if timeout_ms:
        print(f"  Timeout:          {timeout_ms / 1000:.1f}s")

    # If bipartite or tree, and m1/m2 are not specified, we can loop over possible partition sizes
    if graph_type in ["bipartite", "tree"]:
        partitions = []
        if m1 is not None and m2 is not None:
            if m1 + m2 == m:
                partitions = [(m1, m2)]
            else:
                print(f"Error: m1 ({m1}) + m2 ({m2}) must equal m ({m})")
                return None
        else:
            # Generate all valid partitions m1 <= m2
            for r in range(1, m // 2 + 1):
                partitions.append((r, m - r))

        for m1_p, m2_p in partitions:
            print(
                f"\n\033[1;34mTrying bipartite partition sizes ({m1_p}, {m2_p})...\033[0m"
            )
            start_time = time.time()
            s, B, homP, homE = build_leontovich_solver_partition(
                m1_p, m2_p, n, d, graph_type, max_deg
            )
            if timeout_ms is not None:
                s.set("timeout", timeout_ms)

            result = s.check()
            elapsed = time.time() - start_time

            if result == z3.sat:
                print(
                    f"\033[1;32m✓ SATISFIABLE (Found in {elapsed:.3f} seconds with partition {m1_p}x{m2_p})\033[0m"
                )
                model = s.model()

                # Reconstruct full adjacency matrix
                adj_matrix = [[0] * m for _ in range(m)]
                for i in range(m1_p):
                    for j in range(m2_p):
                        val = model[B[i][j]].as_long()
                        adj_matrix[i][m1_p + j] = val
                        adj_matrix[m1_p + j][i] = val

                deg_sequence = [sum(adj_matrix[i]) for i in range(m)]
                hP = model[homP].as_long()
                hE = model[homE].as_long()
                g6 = to_graph6(adj_matrix)

                print("\n\033[1;36mFound Target Graph H:\033[0m")
                print(f"  Graph6:          {g6}")
                print(f"  Vertices (m):    {m} (Partition: {m1_p}, {m2_p})")
                print(f"  Edges:           {sum(deg_sequence) // 2}")
                print(f"  Degree Seq:      {deg_sequence}")
                print(f"  hom(P_{n}, H):     {hP:,}")
                print(f"  hom(E_{n}^({d}), H): {hE:,}")
                print(f"  Difference:      {hP - hE:,} (winner: E_n^({d}))")
                return {"sat": True, "g6": g6, "time": elapsed}
            elif result == z3.unsat:
                print(
                    f"\033[1;31m✗ UNSATISFIABLE ({elapsed:.3f}s for partition {m1_p}x{m2_p})\033[0m"
                )
            else:
                print(
                    f"\033[1;35m? TIMEOUT / UNKNOWN ({elapsed:.3f}s for partition {m1_p}x{m2_p})\033[0m"
                )

        print(
            f"\n\033[1;31m✗ Search complete for all bipartite partitions of size {m}.\033[0m"
        )
        return {"sat": False}

    else:
        # General graph search (unpartitioned)
        start_time = time.time()
        s, A, homP, homE = build_leontovich_solver_general(m, n, d, max_deg)
        if timeout_ms is not None:
            s.set("timeout", timeout_ms)

        result = s.check()
        elapsed = time.time() - start_time

        if result == z3.sat:
            print(f"\033[1;32m✓ SATISFIABLE (Found in {elapsed:.3f} seconds)\033[0m")
            model = s.model()
            adj_matrix = [
                [model[A[i][j]].as_long() for j in range(m)] for i in range(m)
            ]
            deg_sequence = [sum(adj_matrix[i]) for i in range(m)]
            hP = model[homP].as_long()
            hE = model[homE].as_long()
            g6 = to_graph6(adj_matrix)

            print("\n\033[1;36mFound Target Graph H:\033[0m")
            print(f"  Graph6:          {g6}")
            print(f"  Vertices (m):    {m}")
            print(f"  Edges:           {sum(deg_sequence) // 2}")
            print(f"  Degree Seq:      {deg_sequence}")
            print(f"  hom(P_{n}, H):     {hP:,}")
            print(f"  hom(E_{n}^({d}), H): {hE:,}")
            print(f"  Difference:      {hP - hE:,} (winner: E_n^({d}))")
            return {"sat": True, "g6": g6, "time": elapsed}
        elif result == z3.unsat:
            print(f"\033[1;31m✗ UNSATISFIABLE (Proved in {elapsed:.3f} seconds)\033[0m")
            return {"sat": False}
        else:
            print(f"\033[1;35m? TIMEOUT / UNKNOWN ({elapsed:.3f} seconds)\033[0m")
            return {"sat": False}


def main():
    parser = argparse.ArgumentParser(
        description="Pioneer SMT-based search for Leontovich counterexamples using Z3.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "-m",
        "--vertices",
        type=int,
        default=12,
        help="Number of vertices in target graph H",
    )
    parser.add_argument(
        "-n", "--threshold", type=int, default=13, help="Source tree size threshold (n)"
    )
    parser.add_argument(
        "-d", "--depth", type=int, default=2, help="Pendant depth in E_n"
    )
    parser.add_argument(
        "--type",
        choices=["general", "bipartite", "tree"],
        default="bipartite",
        help="Structural constraint on H",
    )
    parser.add_argument(
        "--m1", type=int, default=None, help="Bipartite partition left size (m1)"
    )
    parser.add_argument(
        "--m2", type=int, default=None, help="Bipartite partition right size (m2)"
    )
    parser.add_argument(
        "--max-deg", type=int, default=None, help="Upper bound on vertex degrees in H"
    )
    parser.add_argument(
        "--timeout", type=int, default=30000, help="Solver timeout in milliseconds"
    )

    args = parser.get_args() if hasattr(parser, "get_args") else parser.parse_args()

    run_smt_search(
        m=args.vertices,
        n=args.threshold,
        d=args.depth,
        graph_type=args.type,
        m1=args.m1,
        m2=args.m2,
        max_deg=args.max_deg,
        timeout_ms=args.timeout,
    )


if __name__ == "__main__":
    main()
