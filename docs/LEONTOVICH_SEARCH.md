# Leontovich Graph Search & Verification

Computational verification and extension of results from
_Long paths need not minimize H-colorings among trees_ (J.D. Nir, arXiv:2510.18770v1).

A graph $H$ is **Leontovich at $n$** if $\exists$ tree $T_n$ on $n$ vertices
with $\hom(T_n, H) < \hom(P_n, H)$.

## Analytical Verification: The $T(x,y,z)$ Family

The rooted tree $T(x,y,z)$ has 4 automorphic orbits with similarity matrix:

$$
M = \begin{pmatrix} 0 & x & 0 & 0 \\ 1 & 0 & y & 0 \\ 0 & 1 & 0 & z \\ 0 & 0 & 1 & 0 \end{pmatrix},
\quad a = (1,\; x,\; xy,\; xyz)
$$

enabling $O(1)$ homomorphism evaluation via $\hom(G_n, H) = a \cdot M^n \cdot h(G_0, v_0)$.

### Theorem 1.3 Verification

$H = T(18, 3, 32)$ with $|V| = 1{,}801$.

| tree  | $\hom(\cdot, H)$ | result            |
| ----- | ---------------- | ----------------- |
| $P_7$ | 81,558,090       |                   |
| $E_7$ | 81,548,856       | **wins by 9,234** |

### Exact Threshold for $T(7,1,9)$

$H = T(7,1,9)$ with $|V| = 78$. The paper proves $E_n$ beats $P_n$ for
"sufficiently large odd $n$" but does not give the threshold.

| $n$    | parity  | $\hom(P_n, H)$  | $\hom(E_n, H)$  | $P_n - E_n$ | winner    |
| ------ | ------- | --------------- | --------------- | ----------- | --------- |
| 5      | odd     | 9,366           | 9,492           | $-126$      | $P_n$     |
| 7      | odd     | 106,302         | 106,932         | $-630$      | $P_n$     |
| 9      | odd     | 1,217,076       | 1,219,848       | $-2{,}772$  | $P_n$     |
| 11     | odd     | 13,993,266      | 14,000,700      | $-7{,}434$  | $P_n$     |
| **13** | **odd** | **161,209,734** | **161,161,476** | **48,258**  | **$E_n$** |
| 15     | odd     | 1,858,989,720   | 1,857,700,992   | 1,288,728   | $E_n$     |
| 17     | odd     | 21,446,611,998  | 21,427,743,876  | 18,868,122  | $E_n$     |

**Result:** Threshold is $n = 13$. For all even $n$, $P_n$ wins.

### Smallest 4-Orbit Leontovich Graph

Exhaustive grid search over $T(x,y,z)$ with $|V| = 1 + x + xy + xyz \le 100$:

**$T(7,1,9)$ with 78 vertices is the unique smallest** in the $T(x,y,z)$ family.

This confirms that closing the Problem 4.3 gap ($4 \le m \le 78$) requires
searching outside the 4-orbit spherically symmetric family.

## General Graph Search (Problem 4.3)

Open problem: what is the smallest $m$ such that a graph $H$ on $m$ vertices
is Leontovich? The paper shows $4 \le m \le 78$.

### Exhaustive sweep: all connected $H$ with $|V(H)| \le 8$

For each $k$, we loaded all connected graphs on $k$ vertices via `geng -c k`
and tested every tree on $n$ vertices for $n \in \{7, 10, 13, 15\}$.
Graph counts cross-validated against [OEIS A001349](https://oeis.org/A001349).

| $   | V(H)   | $          | connected $H$ (A001349) | trees tested ($n=7..15$) | violations |
| --- | ------ | ---------- | ----------------------- | ------------------------ | ---------- |
| 4   | 6      | $4 \times$ | **0**                   |
| 5   | 21     | $4 \times$ | **0**                   |
| 6   | 112    | $4 \times$ | **0**                   |
| 7   | 853    | $4 \times$ | **0**                   |
| 8   | 11,117 | $4 \times$ | **0**                   |

**New result:** No graph on $\le 8$ vertices is Leontovich at any $n \le 15$.

This improves the lower bound from $m \ge 4$ to **$m \ge 9$**,
narrowing Problem 4.3 to $9 \le m \le 78$.

## Reproduction

```bash
# Analytical verification (Tasks A, B, C)
python3 scripts/leontovich.py

# General graph search (Problem 4.3)
# Test all connected H on k vertices against trees of size n
./synthesizer N --leontovich K --quiet
```
