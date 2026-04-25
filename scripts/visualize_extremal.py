#!/usr/bin/env python3
"""Generate graphviz .dot files for extremal Δ≤3 trees from sequence.jsonl."""

import json
import os

JSONL = "docs/runs/sequence.jsonl"
OUTDIR = "docs/extremal_trees"
os.makedirs(OUTDIR, exist_ok=True)

# Show a representative sample: small, medium, large
SHOW_N = [7, 10, 15, 20, 24]

with open(JSONL) as f:
    rows = {d["n"]: d for line in f if line.strip() for d in [json.loads(line)]}

for n in SHOW_N:
    if n not in rows:
        print(f"  Skipping N={n} (not in JSONL)")
        continue
    r = rows[n]
    edges = r["d3_edges"]
    score = r["d3"]
    leaves_count = r["d3_leaves"]
    diam = r["d3_diam"]

    dot = f"graph T{n} {{\n"
    dot += f'  label="N={n}, i(T)={score:,}, leaves={leaves_count}, diam={diam}";\n'
    dot += "  labelloc=t;\n"
    dot += '  fontname="Inter";\n'
    dot += "  fontsize=14;\n"
    dot += "  node [shape=circle, style=filled, width=0.35, "
    dot += 'fontsize=10, fontname="Inter"];\n'
    dot += '  edge [color="#555555"];\n\n'

    # Compute degree for coloring
    deg = {}
    for u, v in edges:
        deg[u] = deg.get(u, 0) + 1
        deg[v] = deg.get(v, 0) + 1

    # Color: leaves=light green, degree-2=light blue, degree-3=orange (hubs)
    for i in range(n):
        d = deg.get(i, 0)
        if d == 1:
            color = '"#a8e6cf"'  # leaf - green
        elif d == 2:
            color = '"#dcedc1"'  # spine - light
        else:
            color = '"#ffd3b6"'  # hub - orange
        dot += f'  {i} [fillcolor={color}, label="{i}"];\n'

    dot += "\n"
    for u, v in edges:
        dot += f"  {u} -- {v};\n"
    dot += "}\n"

    outpath = os.path.join(OUTDIR, f"extremal_d3_n{n}.dot")
    with open(outpath, "w") as f:
        f.write(dot)
    print(f"  {outpath}")

# Render if graphviz available
import shutil

if shutil.which("dot"):
    print("\nRendering PNGs...")
    for n in SHOW_N:
        dot_path = os.path.join(OUTDIR, f"extremal_d3_n{n}.dot")
        png_path = os.path.join(OUTDIR, f"extremal_d3_n{n}.png")
        if os.path.exists(dot_path):
            os.system(f'dot -Tpng -Gdpi=200 "{dot_path}" -o "{png_path}"')
            print(f"  {png_path}")
else:
    print("\n  graphviz not found, .dot files ready for manual rendering")
