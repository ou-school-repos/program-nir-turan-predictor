#!/usr/bin/env python3
"""Convert and verify the newly discovered 16-vertex general Leontovich graph."""

from verify_hom import verify_graph


def main():
    edges = [
        [0, 1],
        [0, 2],
        [0, 3],
        [0, 7],
        [0, 11],
        [0, 12],
        [1, 7],
        [1, 8],
        [1, 12],
        [1, 13],
        [2, 3],
        [2, 7],
        [2, 8],
        [2, 9],
        [2, 10],
        [2, 12],
        [3, 4],
        [3, 6],
        [3, 7],
        [3, 8],
        [3, 14],
        [3, 15],
        [4, 5],
        [4, 9],
        [5, 8],
        [5, 9],
        [5, 10],
        [5, 13],
        [6, 12],
        [7, 8],
        [8, 9],
        [8, 11],
        [8, 14],
        [8, 15],
        [9, 12],
        [9, 13],
        [9, 14],
        [10, 11],
        [10, 13],
        [11, 12],
        [12, 13],
        [12, 15],
        [13, 14],
        [13, 15],
    ]
    m = 16

    # Reconstruct Adjacency Matrix
    A = [[0] * m for _ in range(m)]
    for u, v in edges:
        A[u][v] = A[v][u] = 1

    # Convert to Graph6
    g6 = chr(m + 63)
    bits = []
    for col in range(1, m):
        for row in range(col):
            bits.append(A[row][col])

    while len(bits) % 6 != 0:
        bits.append(0)

    for i in range(0, len(bits), 6):
        val = 0
        for b in range(6):
            val = (val << 1) | bits[i + b]
        g6 += chr(val + 63)

    print(f"Generated Graph6: {g6}\n")

    # Run exact integer verification
    verify_graph(g6)


if __name__ == "__main__":
    main()
