#!/usr/bin/env python3
"""Exact checks for the paper's core computational claims.

This script is intentionally small and dependency-free. It verifies the
headline witness values with Python integers:

* H* is a 15-vertex depth-dependent bipartite Leontovich graph.
* H18 has the corrected quotient walk table and n=17 depth-2 margin.
* The 5-orbit Table 8 finite-window audit has the expected open/close windows.
* The m1=2 transfer identity matches exact homomorphism counts on a grid.
* The double-cover source graph T^(1,35,1,50) has first even d=2 crossover
  at n=17340, hence so does its bipartite double cover.
* The printed T^(1,35,1,50) leading-coefficient ratio and high-n finite ratio
  are reproduced.
"""

from __future__ import annotations

from dataclasses import dataclass
from decimal import Decimal, getcontext

from strong_coeff import leading_ratio

SUBSETS = ((0,), (1,), (2,), (0, 1), (0, 2), (1, 2), (0, 1, 2))


def bipartite_walks(
    pattern: tuple[int, ...], max_step: int
) -> tuple[list[int], list[list[int]]]:
    """Return orbit sizes and exact walk vectors for a (3,m2) pattern graph."""
    sizes = [1, 1, 1] + list(pattern)
    w = [[1] * 10]
    for _ in range(max_step):
        prev = w[-1]
        w.append(
            [
                sum(
                    pattern[j] * prev[3 + j]
                    for j, subset in enumerate(SUBSETS)
                    if 0 in subset
                ),
                sum(
                    pattern[j] * prev[3 + j]
                    for j, subset in enumerate(SUBSETS)
                    if 1 in subset
                ),
                sum(
                    pattern[j] * prev[3 + j]
                    for j, subset in enumerate(SUBSETS)
                    if 2 in subset
                ),
                prev[0],
                prev[1],
                prev[2],
                prev[0] + prev[1],
                prev[0] + prev[2],
                prev[1] + prev[2],
                prev[0] + prev[1] + prev[2],
            ]
        )
    return sizes, w


def hom_path(sizes: list[int], walks: list[list[int]], n: int) -> int:
    return sum(size * walks[n - 1][i] for i, size in enumerate(sizes))


def hom_near_path(sizes: list[int], walks: list[list[int]], n: int, d: int) -> int:
    stem = n - d - 2
    if stem < 0:
        raise ValueError(f"invalid near-path parameters n={n}, d={d}")
    return sum(
        size * walks[stem][i] * walks[1][i] * walks[d][i]
        for i, size in enumerate(sizes)
    )


def verify_h_star() -> None:
    pattern = (0, 1, 6, 4, 1, 0, 0)
    sizes, walks = bipartite_walks(pattern, 200)
    delta = hom_path(sizes, walks, 49) - hom_near_path(sizes, walks, 49, 16)
    expected = 41_377_493_496_440_451_164
    assert delta == expected, delta

    hits = []
    for d in range(2, 21):
        for n in range(5, 200, 2):
            if n - d - 2 >= 0:
                dlt = hom_path(sizes, walks, n) - hom_near_path(sizes, walks, n, d)
                if dlt > 0:
                    hits.append((n, d))
    assert len(hits) == 300, len(hits)
    assert min(hits) == (49, 16), min(hits)
    assert {d for _, d in hits} == {14, 16, 18, 20}
    assert all(
        hom_path(sizes, walks, n) <= hom_near_path(sizes, walks, n, 2)
        for n in range(5, 200, 2)
    )
    print("H*: exact 15-vertex depth-dependent witness verified")


def verify_h18() -> None:
    pattern = (7, 0, 0, 1, 1, 6, 0)
    sizes, walks = bipartite_walks(pattern, 20)
    orbit_order = [0, 1, 2, 3, 6, 7, 8]
    expected_rows = [
        [1, 1, 1, 1, 1, 1, 1, 18],
        [9, 7, 7, 1, 2, 2, 2, 46],
        [11, 14, 14, 9, 16, 16, 14, 218],
        [95, 100, 100, 11, 25, 25, 28, 590],
        [127, 193, 193, 95, 195, 195, 200, 2768],
        [1055, 1395, 1395, 127, 320, 320, 386, 7690],
    ]
    for step, expected in enumerate(expected_rows):
        row = [walks[step][i] for i in orbit_order]
        row.append(hom_path(sizes, walks, step + 1) if step > 0 else sum(sizes))
        assert row == expected, (step, row, expected)

    hp = hom_path(sizes, walks, 17)
    he = hom_near_path(sizes, walks, 17, 2)
    assert hp == 14_801_051_732, hp
    assert he == 14_795_982_954, he
    assert hp - he == 5_068_778
    print("H18: walk table and n=17 depth-2 margin verified")


def verify_m1_equals_2_identity() -> None:
    """Check the closed-form m1=2 identity against exact quotient walks."""
    for c1 in range(8):
        for c2 in range(8):
            for c3 in range(1, 8):
                sizes = [1, 1, c1, c2, c3]
                w = [[1] * 5]
                for _ in range(5):
                    prev = w[-1]
                    w.append(
                        [
                            c1 * prev[2] + c3 * prev[4],
                            c2 * prev[3] + c3 * prev[4],
                            prev[0],
                            prev[1],
                            prev[0] + prev[1],
                        ]
                    )

                hp = hom_path(sizes, w, 5)
                he = hom_near_path(sizes, w, 5, 2)
                expected = -c3 * ((c1 - c2) ** 2 + c1 + c2)
                assert hp - he == expected, (c1, c2, c3, hp - he, expected)
    print("m1=2 identity: exact finite-grid check verified")


@dataclass(frozen=True)
class LoopedSymmetricTree:
    degrees: tuple[int, ...]

    @property
    def sizes(self) -> list[int]:
        out = [1]
        for d in self.degrees:
            out.append(out[-1] * d)
        return out

    def walks(self, max_step: int) -> list[list[int]]:
        dim = len(self.degrees) + 1
        w = [[1] * dim]
        for _ in range(max_step):
            prev = w[-1]
            nxt = [0] * dim
            nxt[0] = prev[0] + self.degrees[0] * prev[1]
            for i in range(1, dim - 1):
                nxt[i] = prev[i - 1] + self.degrees[i] * prev[i + 1]
            nxt[dim - 1] = prev[dim - 2]
            w.append(nxt)
        return w


def tree_delta(
    tree: LoopedSymmetricTree, walks: list[list[int]], n: int, d: int = 2
) -> int:
    sizes = tree.sizes
    hp = sum(size * walks[n - 1][i] for i, size in enumerate(sizes))
    he = sum(
        size * walks[n - d - 2][i] * walks[1][i] * walks[d][i]
        for i, size in enumerate(sizes)
    )
    return hp - he


def tree_counts(
    tree: LoopedSymmetricTree, walks: list[list[int]], n: int, d: int = 2
) -> tuple[int, int]:
    sizes = tree.sizes
    hp = sum(size * walks[n - 1][i] for i, size in enumerate(sizes))
    he = sum(
        size * walks[n - d - 2][i] * walks[1][i] * walks[d][i]
        for i, size in enumerate(sizes)
    )
    return hp, he


def crossover_flips(tree: LoopedSymmetricTree, max_n: int = 17501) -> list[int]:
    walks = tree.walks(max_n)
    flips: list[int] = []
    prev = None
    for n in range(5, max_n - 3, 2):
        sign = 1 if tree_delta(tree, walks, n) > 0 else -1
        if prev is not None and sign != prev:
            flips.append(n)
        prev = sign
    return flips


def verify_table8() -> None:
    expected = {
        (1, 28, 1, 36): [7, 1827],
        (1, 16, 1, 21): [9, 251],
        (1, 13, 1, 18): [11, 141],
        (1, 12, 1, 18): [13, 127],
        (1, 17, 1, 30): [15, 261],
        (1, 15, 1, 26): [17, 133],
    }
    for degrees, flips in expected.items():
        actual = crossover_flips(LoopedSymmetricTree(degrees))
        assert actual == flips, (degrees, actual, flips)
    print("Table 8: finite-window open/close crossover audit verified")


def verify_even_crossover() -> None:
    tree = LoopedSymmetricTree((1, 35, 1, 50))
    max_n = 20_000
    walks = tree.walks(max_n)
    first = None
    previous_positive = False
    later_flips = []
    for n in range(4, max_n + 1, 2):
        positive = tree_delta(tree, walks, n) > 0
        if positive and first is None:
            first = n
        if first is not None and positive != previous_positive and n != first:
            later_flips.append(n)
        previous_positive = positive
    assert first == 17_340, first
    assert not later_flips, later_flips[:5]
    assert tree_delta(tree, walks, 17_338) < 0
    assert tree_delta(tree, walks, 17_340) > 0
    print("Even crossover: n=17340 threshold verified through n=20000")


def verify_t135_ratio() -> None:
    tree = LoopedSymmetricTree((1, 35, 1, 50))
    rho = Decimal(str(leading_ratio((1, 35, 1, 50))))
    assert rho < Decimal(1)
    assert rho.quantize(Decimal("0.000000000001")) == Decimal("0.999953714414")

    n = 15_001
    walks = tree.walks(n)
    hp, he = tree_counts(tree, walks, n)
    getcontext().prec = 40
    ratio = Decimal(he) / Decimal(hp)
    assert ratio < Decimal(1)
    assert ratio.quantize(Decimal("0.000001")) == Decimal("0.999865"), ratio
    print(
        "T^(1,35,1,50): leading and high-n ratio checks verified "
        f"(rho={rho:.12f}, n={n}: {ratio:.12f})"
    )


def main() -> None:
    verify_h_star()
    verify_h18()
    verify_m1_equals_2_identity()
    verify_table8()
    verify_even_crossover()
    verify_t135_ratio()
    print("All core exact checks passed.")


if __name__ == "__main__":
    main()
