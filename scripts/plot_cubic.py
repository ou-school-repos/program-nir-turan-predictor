#!/usr/bin/env python3
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import networkx as nx  # noqa: E402

# The exact Graph6 string synthesized by CEGIS
g6_string = "GcWsjO"
G = nx.from_graph6_bytes(g6_string.encode("ascii"))

# Create output directory
os.makedirs("docs", exist_ok=True)

# Plot the graph
plt.figure(figsize=(8, 8))

# Circular layout perfectly highlights regular/symmetric graphs
pos = nx.circular_layout(G)

nx.draw(
    G,
    pos,
    with_labels=True,
    node_color="#ff4d4d",
    node_size=1500,
    font_color="white",
    font_weight="bold",
    font_family="sans-serif",
    edge_color="#adb5bd",
    width=3.0,
)

plt.title(
    "CEGIS Synthesized Optimal Network\n(8-Node, 12-Edge Cubic/3-Regular Graph)",
    fontsize=16,
    pad=20,
    fontfamily="sans-serif",
)

output_path = "docs/cegis_optimal_cubic.png"
plt.savefig(output_path, dpi=300, bbox_inches="tight", transparent=True)
print(f"Graph successfully plotted and saved to: {output_path}")
