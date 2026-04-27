#!/usr/bin/env python3
"""Plot the 76-vertex Leontovich graph T(7, 1, [9^5, 8^2]).

Usage:
    python3 scripts/plot_76.py              # -> docs/out/leontovich_76.gif
    python3 scripts/plot_76.py --png        # -> docs/out/leontovich_76.png
"""

import argparse
import json
import math
from pathlib import Path

import matplotlib
import networkx as nx

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import Patch  # noqa: E402

# ── Load graph from shared JSON ─────────────────────────────────────────
DATA_PATH = Path(__file__).resolve().parent.parent / "data" / "leontovich_76.json"
with open(DATA_PATH) as f:
    GRAPH_DATA = json.load(f)

EDGES = GRAPH_DATA["edges"]

BRANCH_LABELS = ["9L", "9L", "9L", "9L", "9L", "8L", "8L"]


def radial_layout(G):
    """Compute a radial layout: root -> bridges -> hubs -> leaves."""
    pos = {0: (0, 0)}
    for i in range(7):
        angle = 2 * math.pi * i / 7 - math.pi / 2
        br = i + 1
        hub = i + 8
        pos[br] = (2.0 * math.cos(angle), 2.0 * math.sin(angle))
        hx, hy = 4.0 * math.cos(angle), 4.0 * math.sin(angle)
        pos[hub] = (hx, hy)

        neighbors = [n for n in G.neighbors(hub) if n > 14]
        num_leaves = len(neighbors)
        spread = 0.55
        for j, leaf in enumerate(neighbors):
            t = (j - (num_leaves - 1) / 2) / max(num_leaves - 1, 1)
            la = angle + spread * t
            pos[leaf] = (hx + 2.0 * math.cos(la), hy + 2.0 * math.sin(la))
    return pos


def plot(out_path, fmt):
    G = nx.Graph()
    G.add_edges_from(EDGES)

    colors, sizes = [], []
    for v in G.nodes():
        if v == 0:
            colors.append("#FF4444")
            sizes.append(700)
        elif v <= 7:
            colors.append("#44AAFF")
            sizes.append(350)
        elif v <= 14:
            colors.append("#FFAA00")
            sizes.append(600)
        else:
            colors.append("#88CC88")
            sizes.append(120)

    pos = radial_layout(G)

    fig, ax = plt.subplots(1, 1, figsize=(16, 16))
    ax.set_facecolor("white")
    fig.patch.set_facecolor("white")

    nx.draw_networkx_edges(G, pos, ax=ax, edge_color="#999999", width=1.2, alpha=0.6)
    nx.draw_networkx_nodes(
        G,
        pos,
        ax=ax,
        node_color=colors,
        node_size=sizes,
        edgecolors="black",
        linewidths=0.8,
    )

    labels = {0: "root"}
    for i in range(7):
        labels[i + 1] = f"b{i + 1}"
        labels[i + 8] = BRANCH_LABELS[i]

    nx.draw_networkx_labels(
        G,
        pos,
        labels,
        ax=ax,
        font_size=11,
        font_color="black",
        font_weight="bold",
    )

    ax.set_title(
        r"$T(7, 1, [9^5, 8^2])$  —  76-vertex Leontovich graph"
        r"  ($\lambda_2^* \approx 3.038$)",
        color="black",
        fontsize=20,
        pad=25,
        fontweight="bold",
    )

    legend_elements = [
        Patch(facecolor="#FF4444", edgecolor="black", label="Root (deg 7)"),
        Patch(facecolor="#44AAFF", edgecolor="black", label="Bridges (deg 2)"),
        Patch(facecolor="#FFAA00", edgecolor="black", label="Hubs (deg 9-10)"),
        Patch(facecolor="#88CC88", edgecolor="black", label="Leaves (deg 1)"),
    ]
    ax.legend(
        handles=legend_elements,
        loc="upper right",
        fontsize=13,
        facecolor="white",
        edgecolor="#999999",
        labelcolor="black",
    )

    ax.set_xlim(-7, 7)
    ax.set_ylim(-7, 7)
    ax.set_aspect("equal")
    ax.axis("off")
    plt.tight_layout()

    if fmt == "gif":
        tmp = "/tmp/leo76_tmp.png"
        plt.savefig(tmp, dpi=150, bbox_inches="tight", facecolor=fig.get_facecolor())
        from PIL import Image

        Image.open(tmp).save(out_path)
    else:
        plt.savefig(
            out_path, dpi=150, bbox_inches="tight", facecolor=fig.get_facecolor()
        )

    print(f"Saved {out_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot the 76-vertex graph")
    parser.add_argument("--png", action="store_true", help="Output PNG instead of GIF")
    args = parser.parse_args()

    fmt = "png" if args.png else "gif"
    out = f"docs/out/leontovich_76.{fmt}"
    plot(out, fmt)
