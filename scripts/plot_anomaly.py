#!/usr/bin/env python3
"""Render the depth-anomaly figures used in the paper."""

from __future__ import annotations

from pathlib import Path

import matplotlib

matplotlib.use("Agg")
matplotlib.rcParams["pdf.fonttype"] = 42
matplotlib.rcParams["ps.fonttype"] = 42

import matplotlib.pyplot as plt  # noqa: E402
import networkx as nx  # noqa: E402

GRAPH6 = "NCQCCA?_B?K?W?g?K??"
OUTDIR = Path("docs/out")


def parse_graph6(g6: str) -> nx.Graph:
    """Decode a graph6 string into a simple graph."""
    s = g6.strip()
    if s.startswith(">>graph6<<"):
        s = s[10:]
    n = ord(s[0]) - 63
    bits = []
    for ch in s[1:]:
        val = ord(ch) - 63
        for shift in range(5, -1, -1):
            bits.append((val >> shift) & 1)

    G = nx.Graph()
    G.add_nodes_from(range(n))
    k = 0
    for col in range(1, n):
        for row in range(col):
            if bits[k]:
                G.add_edge(row, col)
            k += 1
    return G


def bipartition(G: nx.Graph) -> tuple[list[int], list[int]]:
    """Return the two color classes of the connected bipartite graph."""
    color = {}
    left = []
    right = []
    for start in G.nodes():
        if start in color:
            continue
        color[start] = 0
        stack = [start]
        while stack:
            u = stack.pop()
            for v in G.neighbors(u):
                if v not in color:
                    color[v] = 1 - color[u]
                    stack.append(v)
                elif color[v] == color[u]:
                    raise ValueError("graph is not bipartite")
    for node, c in color.items():
        (left if c == 0 else right).append(node)
    return sorted(left), sorted(right)


def walk_vectors(G: nx.Graph, max_step: int) -> list[list[int]]:
    """Compute exact walk vectors w[k] = A^k * 1."""
    nodes = sorted(G.nodes())
    idx = {node: i for i, node in enumerate(nodes)}
    adj = [[idx[v] for v in G.neighbors(u)] for u in nodes]

    w = [[1] * len(nodes)]
    for _ in range(max_step):
        prev = w[-1]
        w.append([sum(prev[v] for v in adj[i]) for i in range(len(nodes))])
    return w


def hom_path(walks: list[list[int]], n: int) -> int:
    return sum(walks[n - 1])


def hom_near_path(walks: list[list[int]], n: int, d: int) -> int:
    stem = n - d - 2
    return sum(walks[stem][i] * walks[1][i] * walks[d][i] for i in range(len(walks[0])))


def plot_graph(G: nx.Graph) -> None:
    """Render the witness graph as a labeled bipartite drawing."""
    left, right = bipartition(G)
    pos = nx.spring_layout(G, seed=7, k=0.75, iterations=300)

    fig, ax = plt.subplots(figsize=(10.8, 10.6))
    ax.set_facecolor("white")
    fig.patch.set_facecolor("white")

    colors = ["#ef4c3c" if n in left else "#46a3e6" for n in G.nodes()]
    nx.draw_networkx_edges(G, pos, ax=ax, edge_color="#555555", width=1.8, alpha=0.9)
    nx.draw_networkx_nodes(
        G,
        pos,
        ax=ax,
        node_color=colors,
        node_size=1300,
        edgecolors="black",
        linewidths=1.4,
    )
    nx.draw_networkx_labels(
        G,
        pos,
        labels={n: str(n) for n in G.nodes()},
        ax=ax,
        font_size=12,
        font_color="white",
        font_weight="bold",
    )

    ax.set_title("Depth-anomaly graph $H^*$", fontsize=20, pad=20)
    ax.axis("off")
    plt.tight_layout()
    plt.savefig(OUTDIR / "anomaly_NCQCCA.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_ratios(G: nx.Graph) -> None:
    """Render the exact near-path ratio sweep."""
    walks = walk_vectors(G, 200)
    fig, ax = plt.subplots(figsize=(10.0, 6.0))

    depths = [2, 5, 10, 14, 16, 20]
    for d in depths:
        xs = list(range(5, 201, 2))
        ys = [hom_near_path(walks, n, d) / hom_path(walks, n) for n in xs]
        ax.plot(xs, ys, linewidth=2.2, label=f"d={d}")

    ax.axhline(
        1.0,
        color="#555555",
        linestyle="--",
        linewidth=1.5,
        label="ratio = 1 (path wins)",
    )
    ax.axhspan(0.9985, 1.0, color="#f6cdd4", alpha=0.5)
    ax.text(
        171,
        0.99905,
        "anomaly\nzone",
        color="#d94841",
        fontsize=14,
        style="italic",
        ha="center",
    )

    ax.set_title("Near-path ratio for depth-anomaly graph $H^*$", fontsize=18, pad=10)
    ax.set_xlabel("n (path length)", fontsize=16)
    ax.set_ylabel(
        r"$\mathrm{hom}(E_n^{(d)}, H^*) / \mathrm{hom}(P_n, H^*)$", fontsize=16
    )
    ax.set_xlim(20, 200)
    ax.set_ylim(0.9985, 1.004)
    ax.grid(True, alpha=0.25)
    ax.legend(loc="upper right", fontsize=11, ncol=2, framealpha=0.95)
    plt.tight_layout()
    plt.savefig(OUTDIR / "anomaly_ratio.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


def main() -> None:
    OUTDIR.mkdir(parents=True, exist_ok=True)
    G = parse_graph6(GRAPH6)
    plot_graph(G)
    plot_ratios(G)
    print("Saved docs/out/anomaly_NCQCCA.png and docs/out/anomaly_ratio.png")


if __name__ == "__main__":
    main()
