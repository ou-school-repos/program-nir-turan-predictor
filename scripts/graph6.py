"""Shared graph6 helpers for repository synthesis scripts."""


def to_graph6(adj_matrix, n):
    """Convert an adjacency matrix to the graph6 preset string Dendro uses."""
    if n > 62:
        raise ValueError("graph6 encoder only supports n <= 62")

    bits = []
    for j in range(1, n):
        for i in range(j):
            bits.append(adj_matrix[i][j])

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
