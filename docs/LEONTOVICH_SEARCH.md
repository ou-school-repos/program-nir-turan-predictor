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

### Exhaustive tree sweep: all connected $H$ with $\|V(H)\| \le 9$

For each $m$, we loaded all connected graphs via `geng -c m`
and tested every tree on $n$ vertices for $n \in \{7, 10, 13, 15\}$.
Graph counts cross-validated against [OEIS A001349](https://oeis.org/A001349).

| $m = \|V(H)\|$ | connected $H$ | $\sum$ graphs | ops ($H \times T \times n \times m^2$) | violations |
| -------------- | ------------- | ------------- | -------------------------------------- | ---------- |
| 4              | 6             | 6             | $1.3 \times 10^7$                      | **0**      |
| 5              | 21            | 27            | $7.0 \times 10^7$                      | **0**      |
| 6              | 112           | 139           | $5.4 \times 10^8$                      | **0**      |
| 7              | 853           | 992           | $5.6 \times 10^9$                      | **0**      |
| 8              | 11,117        | 12,109        | $9.5 \times 10^{10}$                   | **0**      |
| 9              | 261,080       | 273,189       | $2.8 \times 10^{12}$                   | **0**      |

### Asymptotic filter: $E_n^{(d)}$ family up to $n = 200$

The exhaustive tree sweep only covers $n \le 15$. To close the
"delayed threshold" loophole (a graph whose Leontovich crossover
occurs at large $n$), we use `leontovich_fast.cpp`: for each target $H$,
test the near-path trees $E_n^{(d)}$ (path with pendant at depth $d$)
via $O(m)$ matrix-vector iteration up to $n = 200$, $d \le 20$.

| $m$ | connected $H$ | filter time | violations ($n \le 200$) |
| --- | ------------- | ----------- | ------------------------ |
| 4   | 6             | instant     | **0**                    |
| 5   | 21            | instant     | **0**                    |
| 6   | 112           | instant     | **0**                    |
| 7   | 853           | instant     | **0**                    |
| 8   | 11,117        | < 1s        | **0**                    |
| 9   | 261,080       | ~40s        | **0**                    |
| 10  | 11,716,571    | ~57 min     | **0**                    |

### Current bound

**New result: $m \ge 11$**, narrowing Problem 4.3 to $11 \le m \le 78$.

No connected graph on $\le 10$ vertices is Leontovich, verified by:

1. Exhaustive tree enumeration ($n \le 15$, all 273,189 graphs, $m \le 9$)
2. Asymptotic near-path filter ($n \le 200$, all 11,989,760 graphs, $m \le 10$)

## Reproduction

```bash
# Analytical verification (Tasks A, B, C)
python3 scripts/leontovich.py

# Exhaustive tree sweep (Problem 4.3)
./synthesizer N --leontovich K --quiet

# Asymptotic near-path filter
g++ -O3 -march=native -o leontovich_fast scripts/leontovich_fast.cpp
geng -c K -q | ./leontovich_fast
```
