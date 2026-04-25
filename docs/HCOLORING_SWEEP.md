# H-Coloring Minimizer Sweep: Odd Path Conjecture

## Problem Reference

**Problem 6.3** (Galvin, McMillon, Nir 2026 — _Hoffman-London graphs_):

> Classify the trees that minimize $P_{2m+1}$-colorings among all trees on $n$ vertices.

It is known that even paths $P_{2m}$ are minimized by the path $P_n$ among trees.
The odd case is **open**.

## Computational Evidence

Exhaustive brute-force over all non-isomorphic trees on $n$ vertices.
Engine: `synthesizer.cpp` with `--hcolor` mode (tree DP, $O(n \cdot h^2)$ per tree).

### Phase 1: Even Paths (Cross-Validation)

Confirms the known theorem: $P_n$ minimizes $\hom(T, P_{2m})$ among all trees $T$ on $n$ vertices.

| Target | $n$ range | Trees checked | Violations | Status |
| ------ | --------- | ------------- | ---------- | ------ |
| $P_2$  | 0--20     | 1,643,856     | 0          | Known  |
| $P_4$  | 0--20     | 1,643,856     | 0          | Known  |
| $P_6$  | 0--20     | 1,643,856     | 0          | Known  |

### Phase 2: Odd Paths (Open Problem 6.3)

**Result: No violations found.** The path $P_n$ minimizes $\hom(T, P_{2m+1})$ across all tested cases.

#### $P_3$ Minimizer Sequence

| $n$ | $\min_T \hom(T, P_3)$ | Minimizer |
| --- | --------------------- | --------- |
| 1   | 3                     | $P_1$     |
| 2   | 4                     | $P_2$     |
| 3   | 6                     | $P_3$     |
| 4   | 8                     | $P_4$     |
| 5   | 12                    | $P_5$     |
| 6   | 16                    | $P_6$     |
| 7   | 24                    | $P_7$     |
| 8   | 32                    | $P_8$     |
| 9   | 48                    | $P_9$     |
| 10  | 64                    | $P_{10}$  |
| 11  | 96                    | $P_{11}$  |
| 12  | 128                   | $P_{12}$  |
| 13  | 192                   | $P_{13}$  |
| 14  | 256                   | $P_{14}$  |
| 15  | 384                   | $P_{15}$  |
| 16  | 512                   | $P_{16}$  |
| 17  | 768                   | $P_{17}$  |
| 18  | 1024                  | $P_{18}$  |
| 19  | 1536                  | $P_{19}$  |
| 20  | 2048                  | $P_{20}$  |
| 21  | 3072                  | $P_{21}$  |
| 22  | 4096                  | $P_{22}$  |

**Pattern:** $\hom(P_n, P_3) = 3 \cdot 2^{\lfloor n/2 \rfloor} \cdot (4/3)^{n \bmod 2}$. Alternates $\times \frac{3}{2}$ and $\times \frac{4}{3}$.

#### $P_5$ Minimizer Sequence

| $n$ | $\min_T \hom(T, P_5)$ | Minimizer |
| --- | --------------------- | --------- |
| 1   | 5                     | $P_1$     |
| 2   | 8                     | $P_2$     |
| 3   | 14                    | $P_3$     |
| 4   | 24                    | $P_4$     |
| 5   | 42                    | $P_5$     |
| 6   | 72                    | $P_6$     |
| 7   | 126                   | $P_7$     |
| 8   | 216                   | $P_8$     |
| 9   | 378                   | $P_9$     |
| 10  | 648                   | $P_{10}$  |
| 11  | 1134                  | $P_{11}$  |
| 12  | 1944                  | $P_{12}$  |
| 13  | 3402                  | $P_{13}$  |
| 14  | 5832                  | $P_{14}$  |
| 15  | 10206                 | $P_{15}$  |
| 16  | 17496                 | $P_{16}$  |
| 17  | 30618                 | $P_{17}$  |
| 18  | 52488                 | $P_{18}$  |
| 19  | 91854                 | $P_{19}$  |
| 20  | 157464                | $P_{20}$  |
| 21  | 275562                | $P_{21}$  |
| 22  | 472392                | $P_{22}$  |

**Pattern:** Ratios alternate between $\frac{7}{4} = 1.75$ and $\frac{12}{7} \approx 1.714$.

#### $P_7$ Minimizer Sequence

| $n$ | $\min_T \hom(T, P_7)$ | Minimizer |
| --- | --------------------- | --------- |
| 1   | 7                     | $P_1$     |
| 2   | 12                    | $P_2$     |
| 3   | 22                    | $P_3$     |
| 4   | 40                    | $P_4$     |
| 5   | 74                    | $P_5$     |
| 6   | 136                   | $P_6$     |
| 7   | 252                   | $P_7$     |
| 8   | 464                   | $P_8$     |
| 9   | 860                   | $P_9$     |
| 10  | 1584                  | $P_{10}$  |
| 11  | 2936                  | $P_{11}$  |
| 12  | 5408                  | $P_{12}$  |
| 13  | 10024                 | $P_{13}$  |
| 14  | 18464                 | $P_{14}$  |
| 15  | 34224                 | $P_{15}$  |
| 16  | 63040                 | $P_{16}$  |
| 17  | 116848                | $P_{17}$  |
| 18  | 215232                | $P_{18}$  |
| 19  | 398944                | $P_{19}$  |
| 20  | 734848                | $P_{20}$  |
| 21  | 1362080               | $P_{21}$  |
| 22  | 2508928               | $P_{22}$  |

#### $P_9$ Minimizer Sequence

| $n$ | $\min_T \hom(T, P_9)$ | Minimizer |
| --- | --------------------- | --------- |
| 1   | 9                     | $P_1$     |
| 2   | 16                    | $P_2$     |
| 3   | 30                    | $P_3$     |
| 4   | 56                    | $P_4$     |
| 5   | 106                   | $P_5$     |
| 6   | 200                   | $P_6$     |
| 7   | 380                   | $P_7$     |
| 8   | 720                   | $P_8$     |
| 9   | 1370                  | $P_9$     |
| 10  | 2600                  | $P_{10}$  |
| 11  | 4950                  | $P_{11}$  |
| 12  | 9400                  | $P_{12}$  |
| 13  | 17900                 | $P_{13}$  |
| 14  | 34000                 | $P_{14}$  |
| 15  | 64750                 | $P_{15}$  |
| 16  | 123000                | $P_{16}$  |
| 17  | 234250                | $P_{17}$  |
| 18  | 445000                | $P_{18}$  |
| 19  | 847500                | $P_{19}$  |
| 20  | 1610000               | $P_{20}$  |
| 21  | 3066250               | $P_{21}$  |
| 22  | 5825000               | $P_{22}$  |

### Summary

| Target | $n$ range | Total trees | Violations | Path minimizes? |
| ------ | --------- | ----------- | ---------- | --------------- |
| $P_3$  | 0--22     | 7,291,468   | **0**      | Yes             |
| $P_5$  | 0--22     | 7,291,468   | **0**      | Yes             |
| $P_7$  | 0--22     | 7,291,468   | **0**      | Yes             |
| $P_9$  | 0--22     | 7,291,468   | **0**      | Yes             |

**Conjecture (computational):** For all odd paths $P_{2m+1}$ and all trees $T$ on $n$ vertices:

$$\hom(T, P_{2m+1}) \geq \hom(P_n, P_{2m+1})$$

Verified exhaustively for $m \in \{1, 2, 3, 4\}$ and $n \leq 22$ (5,623,756 trees at $n=22$).

## Independent Verification

The following data enables hand-verification of our results.

### Tree Counts (OEIS A000055)

The number of non-isomorphic trees on $n$ vertices matches the well-known sequence:

| $n$ | Trees (A000055) |
| --- | --------------- |
| 5   | 3               |
| 10  | 106             |
| 15  | 7,741           |
| 20  | 823,065         |
| 22  | 5,623,756       |

### Star Upper Bound: $\hom(K_{1,n-1}, P_k)$

For $P_k$ a path on $k$ vertices, the star $K_{1,n-1}$ gives:
$\hom(K_{1,n-1}, P_k) = \sum_{c=0}^{k-1} (\deg_{P_k}(c))^{n-1}$.

These values can be computed by hand and should match our engine output.

| $n$ | $\hom(K_{1,n-1}, P_3)$ | $\hom(K_{1,n-1}, P_5)$ | $\hom(K_{1,n-1}, P_7)$ | $\hom(K_{1,n-1}, P_9)$ |
| --- | ---------------------- | ---------------------- | ---------------------- | ---------------------- |
| 5   | 18                     | 50                     | 82                     | 114                    |
| 10  | 514                    | 1,538                  | 2,562                  | 3,586                  |
| 15  | 16,386                 | 49,154                 | 81,922                 | 114,690                |
| 20  | 524,290                | 1,572,866              | 2,621,442              | 3,670,018              |

### Aggregate Checksum: $\sum_T \hom(T, P_k)$

Sum over all trees $T$ on $n$ vertices. Impossible to forge without enumerating every tree.

| $n$ | $\sum_T \hom(T, P_3)$ | $\sum_T \hom(T, P_5)$ | $\sum_T \hom(T, P_7)$ | $\sum_T \hom(T, P_9)$ |
| --- | --------------------- | --------------------- | --------------------- | --------------------- |
| 5   | 42                    | 136                   | 232                   | 328                   |
| 10  | 10,106                | 89,684                | 196,526               | 305,044               |
| 15  | 4,848,990             | 120,740,218           | 351,626,300           | 603,788,228           |
| 20  | 3,358,401,446         | 236,529,883,790       | 923,473,405,780       | 1,764,733,470,872     |

### Podium (Top-3 Distinct Scores + Tie Counts)

Shows the 3 smallest distinct values of $\hom(T, P_k)$ and how many trees achieve each.
If path is the unique minimizer, the #1 tie count should be 1.

#### $P_3$ (ties expected)

| $n$ | total   | #1 score | #1 ties           | #2 score | #2 ties | #3 score | #3 ties |
| --- | ------- | -------- | ----------------- | -------- | ------- | -------- | ------- |
| 5   | 3       | 12       | **2** (67%)       | 18       | 1       | --       | --      |
| 10  | 106     | 64       | **37** (35%)      | 80       | 45      | 136      | 19      |
| 15  | 7,741   | 384      | **3,927** (51%)   | 576      | 2,472   | 1,056    | 1,008   |
| 20  | 823,065 | 2,048    | **196,096** (24%) | 2,560    | 326,893 | 4,352    | 191,045 |

#### $P_5$

| $n$ | total   | #1 score | #1 ties | #2 score | #2 ties | #3 score | #3 ties |
| --- | ------- | -------- | ------- | -------- | ------- | -------- | ------- |
| 5   | 3       | 42       | 1       | 44       | 1       | 50       | 1       |
| 10  | 106     | 648      | 1       | 684      | 4       | 702      | 3       |
| 15  | 7,741   | 10,206   | 1       | 10,692   | 12      | 10,854   | 4       |
| 20  | 823,065 | 157,464  | 1       | 166,212  | 20      | 170,586  | 10      |

#### $P_7$

| $n$ | total   | #1 score | #1 ties | #2 score | #2 ties | #3 score | #3 ties |
| --- | ------- | -------- | ------- | -------- | ------- | -------- | ------- |
| 5   | 3       | 74       | 1       | 76       | 1       | 82       | 1       |
| 10  | 106     | 1,584    | 1       | 1,640    | 1       | 1,648    | 1       |
| 15  | 7,741   | 34,224   | 1       | 35,344   | 2       | 35,536   | 2       |
| 20  | 823,065 | 734,848  | 1       | 760,960  | 1       | 765,440  | 1       |

#### $P_9$

| $n$ | total   | #1 score  | #1 ties | #2 score  | #2 ties | #3 score  | #3 ties |
| --- | ------- | --------- | ------- | --------- | ------- | --------- | ------- |
| 5   | 3       | 106       | 1       | 108       | 1       | 114       | 1       |
| 10  | 106     | 2,600     | 1       | 2,660     | 1       | 2,668     | 1       |
| 15  | 7,741   | 64,750    | 1       | 66,200    | 2       | 66,500    | 2       |
| 20  | 823,065 | 1,610,000 | 1       | 1,648,000 | 1       | 1,652,500 | 1       |

**Key findings:**

- **$P_k$ for $k \ge 5$**: Path is the **unique** minimizer at every $n$ (always 1 tree at #1)
- **$P_3$**: Path is **far from unique** -- at $n=20$, 196,096 of 823,065 trees (24%) tie for #1

### Transfer Matrix for $\hom(P_n, P_5)$

The path baseline can be computed via the recurrence $\mathbf{v}_n = A_{P_5} \cdot \mathbf{v}_{n-1}$, where:

$$A_{P_5} = \begin{pmatrix} 0&1&0&0&0 \\ 1&0&1&0&0 \\ 0&1&0&1&0 \\ 0&0&1&0&1 \\ 0&0&0&1&0 \end{pmatrix}, \quad \mathbf{v}_1 = (1,1,1,1,1)^T$$

$\hom(P_n, P_5) = \mathbf{1}^T \cdot \mathbf{v}_n$. The eigenvalues of $A_{P_5}$ are $2\cos(\pi j/6)$ for $j=1,\ldots,5$, giving dominant growth rate $\sqrt{3} \approx 1.732$.

## Reproduction

```bash
# Build
g++ -O3 -march=native -std=c++17 -o synthesizer src/synthesizer.cpp

# Full sweep (n=0..20, ~3 min)
bash scripts/sweep_hcolor.sh

# Extend to larger n
bash scripts/sweep_hcolor.sh 21 25
```
