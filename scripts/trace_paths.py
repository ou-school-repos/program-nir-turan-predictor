#!/usr/bin/env python3
"""trace_paths.py — Trace paths, walk recurrences, and sample walk sequences on H_18.

This script demonstrates three different levels of path analysis on the
record-breaking 18-vertex Leontovich graph:
  1. All-pairs shortest path routing between representative node groups.
  2. Exact step-by-step walk vector propagation up to step 16 (14.8 billion walks).
  3. Sampling and printing of representative walk sequences of length 16.
"""

import random

import networkx as nx


def get_group_name(v):
    """Retrieve the group name of a vertex."""
    if v == 0:
        return "L0"
    if v == 1:
        return "L1"
    if v == 2:
        return "L2"
    if v in range(3, 10):
        return f"R_leaf_0 (r{v})"
    if v == 10:
        return "R01"
    if v == 11:
        return "R02"
    return f"R12 (r{v})"


def main():
    m = 18
    adj = {i: [] for i in range(m)}

    # Reconstruct H_18 undirected adjacency list
    for r in range(3, 10):
        adj[0].append(r)
        adj[r].append(0)

    for left_node in [0, 1]:
        adj[left_node].append(10)
        adj[10].append(left_node)

    for left_node in [0, 2]:
        adj[left_node].append(11)
        adj[11].append(left_node)

    for r in range(12, 18):
        for left_node in [1, 2]:
            adj[left_node].append(r)
            adj[r].append(left_node)

    G = nx.Graph()
    for u in adj:
        for v in adj[u]:
            G.add_edge(u, v)

    print("=" * 90)
    print("1. SHORTEST PATH ROUTING BETWEEN REPRESENTATIVE SYMMETRY GROUPS")
    print("=" * 90)
    representatives = {
        "L0": 0,
        "L1": 1,
        "L2": 2,
        "R_leaf_0": 3,
        "R01": 10,
        "R02": 11,
        "R12": 12,
    }

    # Print a neat routing matrix of shortest path lengths
    header = f"{'Group':<12} | " + " | ".join(f"{k:<8}" for k in representatives)
    print(header)
    print("-" * len(header))
    for src_name, src_val in representatives.items():
        row = [f"{src_name:<12} |"]
        for dst_name, dst_val in representatives.items():
            path_len = nx.shortest_path_length(G, src_val, dst_val)
            row.append(f"{path_len:<8}")
        print(" ".join(row))

    print("\nSample Routing Paths:")
    print(
        f"  R_leaf_0 to R12: {' -> '.join(get_group_name(node) for node in nx.shortest_path(G, 3, 12))}"
    )
    print(
        f"  L1 to R02:       {' -> '.join(get_group_name(node) for node in nx.shortest_path(G, 1, 11))}"
    )

    print("\n" + "=" * 90)
    print("2. EXACT WALK RECURRENCE TRACE UP TO STEP 16 (P_17 CROSSOVER WINDOW)")
    print("=" * 90)
    # Trace the exact walk vector transitions step by step
    # w[k][u] is the number of walks of length k starting at u
    w = [[1] * m]
    max_steps = 16

    print(
        f"{'Step k':<8} | {'L0':<9} | {'L1':<9} | {'L2':<9} | {'R_leaf_0 (x7)':<13} | "
        f"{'R01 (x1)':<9} | {'R02 (x1)':<9} | {'R12 (x6)':<9} | {'Total Sum':<15}"
    )
    print("-" * 110)

    for step in range(1, max_steps + 1):
        prev = w[-1]
        nxt = [sum(prev[v] for v in adj[u]) for u in range(m)]
        w.append(nxt)

    for k in range(max_steps + 1):
        total_sum = sum(w[k])
        print(
            f"{k:<8d} | {w[k][0]:<9d} | {w[k][1]:<9d} | {w[k][2]:<9d} | {w[k][3]:<13d} | "
            f"{w[k][10]:<9d} | {w[k][11]:<9d} | {w[k][12]:<9d} | {total_sum:<15,d}"
        )

    print("\n" + "=" * 90)
    print("3. SAMPLING INDIVIDUAL REPRESENTATIVE WALK SEQUENCES OF LENGTH 16")
    print("=" * 90)
    print(
        "It is too large combinatorially to print all 14,801,051,732 walks of length 16."
    )
    print("However, we can randomly sample and trace a few individual valid walks:\n")

    # Seed random number generator for consistency
    random.seed(42)

    for run in range(3):
        # Pick a random starting vertex
        current_node = random.randint(0, m - 1)
        walk = [current_node]

        for _ in range(16):
            current_node = random.choice(adj[current_node])
            walk.append(current_node)

        walk_str = " -> ".join(f"{get_group_name(v)}" for v in walk)
        print(f"Sample Walk #{run + 1}:")
        print(f"  Node sequence: {walk}")
        print(f"  Group sequence: {walk_str}\n")


if __name__ == "__main__":
    main()
