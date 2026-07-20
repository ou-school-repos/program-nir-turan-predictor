"""Shared graph6 helpers for repository synthesis scripts."""

DENDRO_MAX_NODES = 32
DENDRO_MAX_EDGES = 63


def to_graph6(adj_matrix, n):
    """Convert an adjacency matrix to the graph6 preset string Dendro uses."""
    if not 1 <= n <= DENDRO_MAX_NODES:
        raise ValueError("Dendro graph6 encoder supports 1-32 nodes")

    bits = []
    edge_count = 0
    for j in range(1, n):
        for i in range(j):
            bit = adj_matrix[i][j]
            edge_count += bit
            bits.append(bit)
    if edge_count > DENDRO_MAX_EDGES:
        raise ValueError("Dendro graph6 encoder supports at most 63 edges")

    while len(bits) % 6 != 0:
        bits.append(0)

    out = [chr(n + 63)]
    for i in range(0, len(bits), 6):
        val = (
            (bits[i] << 5)
            | (bits[i + 1] << 4)
            | (bits[i + 2] << 3)
            | (bits[i + 3] << 2)
            | (bits[i + 4] << 1)
            | bits[i + 5]
        )
        out.append(chr(val + 63))
    return "graph6:" + "".join(out)
