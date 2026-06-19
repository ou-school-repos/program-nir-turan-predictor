#!/usr/bin/env python3
"""Plot the record-breaking 18-vertex Leontovich graph H_18.

Usage:
    python3 scripts/plot_18.py
"""

import math
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
matplotlib.rcParams["pdf.fonttype"] = 42  # TrueType fonts in PDF
matplotlib.rcParams["ps.fonttype"] = 42

import matplotlib.pyplot as plt  # noqa: E402
import networkx as nx  # noqa: E402
from matplotlib.patches import Patch  # noqa: E402


def main():
    m = 18
    # Reconstruct adjacency list for H_18
    adj = {i: [] for i in range(m)}

    # R_leaf_0: 7 leaves connected to L0
    for r in range(3, 10):
        adj[0].append(r)
        adj[r].append(0)

    # R_01: 1 node connected to L0 and L1
    for left_node in [0, 1]:
        adj[left_node].append(10)
        adj[10].append(left_node)

    # R_02: 1 node connected to L0 and L2
    for left_node in [0, 2]:
        adj[left_node].append(11)
        adj[11].append(left_node)

    # R_12: 6 nodes connected to L1 and L2
    for r in range(12, 18):
        for left_node in [1, 2]:
            adj[left_node].append(r)
            adj[r].append(left_node)

    G = nx.Graph()
    for u in adj:
        for v in adj[u]:
            G.add_edge(u, v)

    # Custom coordinates for an elegant bipartite layout
    pos = {}

    # Left partition nodes at x = 0 (L0, L2, L1)
    pos[0] = (0.0, 4.0)
    pos[2] = (0.0, 0.0)
    pos[1] = (0.0, -4.0)

    # Right partition nodes:
    # 1. 7 leaves connected to L0 (nodes 3..9)
    for idx, node in enumerate(range(3, 10)):
        angle = math.pi * 0.35 - (idx * math.pi * 0.11)
        pos[node] = (4.0 * math.cos(angle), 4.0 + 3.5 * math.sin(angle))

    # 2. Node 10 connected to {0, 1} (R_01)
    pos[10] = (2.2, 0.0)

    # 3. Node 11 connected to {0, 2} (R_02)
    pos[11] = (1.8, 2.0)

    # 4. 6 core nodes connected to {1, 2} (nodes 12..17)
    for idx, node in enumerate(range(12, 18)):
        angle = math.pi * 0.25 - (idx * math.pi * 0.10)
        pos[node] = (4.0 * math.cos(angle), -2.0 + 3.5 * math.sin(angle))

    # Node styling
    colors, sizes = [], []
    labels = {}
    for v in range(m):
        if v == 0:
            colors.append("#FF4444")  # Left Node L0 (red)
            sizes.append(800)
            labels[v] = "L0"
        elif v == 1:
            colors.append("#FF4444")  # Left Node L1 (red)
            sizes.append(800)
            labels[v] = "L1"
        elif v == 2:
            colors.append("#FF4444")  # Left Node L2 (red)
            sizes.append(800)
            labels[v] = "L2"
        elif v in range(3, 10):
            colors.append("#88CC88")  # R_leaf_0 (green leaves)
            sizes.append(450)
            labels[v] = f"r{v}"
        elif v == 10:
            colors.append("#9b59b6")  # R_01 cross-connector (purple)
            sizes.append(600)
            labels[v] = "R01"
        elif v == 11:
            colors.append("#e056fd")  # R_02 cross-connector (magenta)
            sizes.append(600)
            labels[v] = "R02"
        else:
            colors.append("#FFAA00")  # R_12 bipartite core (orange)
            sizes.append(500)
            labels[v] = f"r{v}"

    # Setup the plot with light background
    fig, ax = plt.subplots(1, 1, figsize=(15, 12))
    ax.set_facecolor("white")
    fig.patch.set_facecolor("white")

    # Draw elements
    nx.draw_networkx_edges(G, pos, ax=ax, edge_color="#333333", width=1.5, alpha=0.7)
    nx.draw_networkx_nodes(
        G,
        pos,
        ax=ax,
        node_color=colors,
        node_size=sizes,
        edgecolors="black",
        linewidths=1.2,
    )
    nx.draw_networkx_labels(
        G,
        pos,
        labels,
        ax=ax,
        font_size=11,
        font_color="black",
        font_weight="bold",
    )

    # Title & Legend
    ax.set_title(
        r"$H_{18}$  —  Minimal 18-Vertex Bipartite Leontovich Graph",
        color="black",
        fontsize=22,
        pad=25,
        fontweight="bold",
    )

    legend_elements = [
        Patch(
            facecolor="#FF4444",
            edgecolor="black",
            label="Left Partition Nodes (L0, L1, L2)",
        ),
        Patch(
            facecolor="#88CC88",
            edgecolor="black",
            label="R_leaf_0: Leaves of L0 (7 nodes)",
        ),
        Patch(
            facecolor="#9b59b6",
            edgecolor="black",
            label="R_01: Cross-Connector of {L0, L1} (1 node)",
        ),
        Patch(
            facecolor="#e056fd",
            edgecolor="black",
            label="R_02: Cross-Connector of {L0, L2} (1 node)",
        ),
        Patch(
            facecolor="#FFAA00",
            edgecolor="black",
            label="R_12: Core Connected to {L1, L2} (6 nodes)",
        ),
    ]
    ax.legend(
        handles=legend_elements,
        loc="upper right",
        fontsize=12,
        facecolor="#f8f9fa",
        edgecolor="#333333",
        labelcolor="black",
    )

    # Adjust limits to fit comfortably
    ax.set_xlim(-1.5, 6.0)
    ax.set_ylim(-6.5, 10.0)
    ax.set_aspect("equal")
    ax.axis("off")
    plt.tight_layout()

    # Save to different outputs
    out_dir = Path("docs/out")
    out_dir.mkdir(parents=True, exist_ok=True)

    # Save files
    for fmt in ["png", "svg", "pdf"]:
        out_path = out_dir / f"leontovich_18.{fmt}"
        plt.savefig(
            out_path, dpi=200, bbox_inches="tight", facecolor=fig.get_facecolor()
        )
        print(f"Saved {out_path}")

    # Also save as walk_h18 in docs for easy access
    for fmt in ["png", "svg"]:
        out_path = Path("docs") / f"walk_h18.{fmt}"
        plt.savefig(
            out_path, dpi=200, bbox_inches="tight", facecolor=fig.get_facecolor()
        )
        print(f"Saved {out_path}")

    plt.close()


if __name__ == "__main__":
    main()
