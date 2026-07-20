#!/usr/bin/env python3
"""Reproducible certificate for the perturbed non-bipartite candidate B'+e.

B' = T-hat(2,34,1,48) x K2 (bipartite double cover, 6806 vertices).
e  = an edge joining two terminal leaves in the same partition of B'
     (chosen as two leaf-copies in layer 0 under a common ggc parent;
     all such choices are equivalent up to automorphism).

Pipeline (no hand-derived quotient, so no bookkeeping risk):
  1. Build the full 6806-vertex graph B'+e explicitly.
  2. Compute the coarsest equitable partition by color refinement, and
     VERIFY equitability vertex-by-vertex (exact, integer).
  3. Build the exact integer quotient matrix Q (18 cells) with cell sizes s.
     Walk counts from the all-ones vector are exactly captured by any
     equitable partition, so Hom(P_n) = s^T Q^{n-1} 1 and
     Hom(E_n^{(2)}) = (s .* w1 .* w2)^T Q^{n-4} 1 hold exactly.
  4. Exact integer walks: first odd crossover and sign scan to n ~ 4000.
  5. High-precision (50 dps) leading-coefficient ratio on the 18x18 quotient:
         rho = ((s .* w1 .* w2) . u1) / (lambda1^3 * (s . u1)),
     where u1 is the Perron right eigenvector of Q. rho < 1 is the
     strongly-Leontovich (leading-coefficient) condition.
     The same pipeline is validated on the UNPERTURBED B', where rho must
     reproduce the independently known 0.999713.

Caveats stated for reviewers:
  * Step 5 is high-precision floating point, not exact. The margin
    1 - rho ~ 4.8e-5 exceeds the working precision by ~40 orders of
    magnitude; for a fully rigorous certificate wrap the eigenproblem in
    interval arithmetic or add a Rayleigh-residual error bound.
  * Because e breaks bipartiteness only slightly, |lambda2| ~ lambda1 - 2.7e-4
    (the near-surviving -lambda1 mode). Finite-n ratio scans therefore
    converge extremely slowly; the leading-coefficient computation, not a
    finite scan, is the decisive check. Pointwise dominance of E_n for ALL
    large n begins only at astronomically large n (same phenomenon as the
    n=17,340 even crossover of B').

Expected output values:
  quotient dim 18; first odd crossover n=9, single sign flip;
  lambda1(B'+e) = 7.160784...,  Delta lambda1 = +1.77e-4;
  rho(B'+e) = 0.999952140676...  < 1;
  rho(B')   = 0.999713000053...  (validation).
"""

import json
from collections import defaultdict

D1, D2, D3, D4 = 2, 34, 1, 48


def build_perturbed():
    adjH = defaultdict(set)
    nid = 1
    children = []
    for _ in range(D1):
        adjH[0].add(nid)
        adjH[nid].add(0)
        children.append(nid)
        nid += 1
    gcs = []
    for c in children:
        for _ in range(D2):
            adjH[c].add(nid)
            adjH[nid].add(c)
            gcs.append(nid)
            nid += 1
    ggcs = []
    for g in gcs:
        for _ in range(D3):
            adjH[g].add(nid)
            adjH[nid].add(g)
            ggcs.append(nid)
            nid += 1
    leaves = []
    for h in ggcs:
        for _ in range(D4):
            adjH[h].add(nid)
            adjH[nid].add(h)
            leaves.append(nid)
            nid += 1
    NH = nid
    assert NH == 3403
    # double cover; loop at root -> cross edge (0,0)-(0,1)
    adj = defaultdict(set)
    for u in range(NH):
        for v in adjH[u]:
            adj[u].add(v + NH)
            adj[v + NH].add(u)
    adj[0].add(NH)
    adj[NH].add(0)
    # perturbing edge between two same-layer sibling leaves
    l1, l2 = leaves[0], leaves[1]
    adj[l1].add(l2)
    adj[l2].add(l1)
    return adj, 2 * NH


def equitable_quotient(adj, N):
    color = [0] * N
    while True:
        sig, new = {}, [0] * N
        for u in range(N):
            key = (color[u], tuple(sorted(color[v] for v in adj[u])))
            new[u] = sig.setdefault(key, len(sig))
        if new == color:
            break
        color = new
    K = len(set(color))
    cells = defaultdict(list)
    for u in range(N):
        cells[color[u]].append(u)
    sizes = [len(cells[c]) for c in range(K)]
    Q = [[0] * K for _ in range(K)]
    for c in range(K):
        row = defaultdict(int)
        for v in adj[cells[c][0]]:
            row[color[v]] += 1
        for c2, k in row.items():
            Q[c][c2] = k
    # exact equitability verification on every vertex
    for c in range(K):
        for u in cells[c]:
            row = defaultdict(int)
            for v in adj[u]:
                row[color[v]] += 1
            assert all(
                Q[c][c2] == row.get(c2, 0) for c2 in range(K)
            ), "partition not equitable"
    return Q, sizes, K


def exact_scan(Q, sizes, K, max_n=4001):
    w = [[1] * K]
    for _ in range(max_n):
        p = w[-1]
        w.append([sum(Q[i][j] * p[j] for j in range(K)) for i in range(K)])

    def hom_p(n):
        return sum(sizes[i] * w[n - 1][i] for i in range(K))

    def hom_e(n, d=2):
        s = n - d - 2
        return sum(sizes[i] * w[s][i] * w[1][i] * w[d][i] for i in range(K))

    flips, prev = [], None
    for n in range(5, max_n - 3, 2):
        sgn = 1 if hom_p(n) - hom_e(n) > 0 else -1
        if prev is not None and sgn != prev:
            flips.append(n)
        prev = sgn
    return flips, w


def leading_ratio(Q, sizes, K):
    import mpmath as mp

    mp.mp.dps = 50
    S = [mp.mpf(s) for s in sizes]
    B = mp.matrix(K, K)
    for i in range(K):
        for j in range(K):
            B[i, j] = Q[i][j] * mp.sqrt(S[i] / S[j])  # symmetric similarity
    E, V = mp.eigsy(B)
    lam1 = E[K - 1]
    u1 = [V[i, K - 1] / mp.sqrt(S[i]) for i in range(K)]
    if u1[0] < 0:
        u1 = [-x for x in u1]
    assert all(x > 0 for x in u1)
    one = [mp.mpf(1)] * K
    w1 = [sum(Q[i][j] * one[j] for j in range(K)) for i in range(K)]
    w2 = [sum(Q[i][j] * w1[j] for j in range(K)) for i in range(K)]
    num = sum(sizes[i] * w1[i] * w2[i] * u1[i] for i in range(K))
    den = lam1**3 * sum(sizes[i] * u1[i] for i in range(K))
    lam2 = max(abs(E[i]) for i in range(K - 1))
    return lam1, lam2, num / den


def main():
    import mpmath as mp

    adj, N = build_perturbed()
    Q, sizes, K = equitable_quotient(adj, N)
    print(
        f"B'+e: {N} vertices -> equitable quotient with {K} cells "
        f"(verified exactly)"
    )
    flips, _ = exact_scan(Q, sizes, K)
    print(
        f"exact odd-n sign scan (n <= 3997): flips at {flips} "
        f"(expected [9], single opening)"
    )
    lam1, lam2, rho = leading_ratio(Q, sizes, K)
    print(f"lambda1 = {mp.nstr(lam1, 12)}   |lambda2| = {mp.nstr(lam2, 12)}")
    print(
        f"leading-coefficient ratio rho = {mp.nstr(rho, 12)}  "
        f"({'<1: strongly Leontovich (coefficient sense)' if rho < 1 else '>=1'})"
    )

    # validation on unperturbed B' via its known 10-cell quotient
    QH = [
        [1, D1, 0, 0, 0],
        [1, 0, D2, 0, 0],
        [0, 1, 0, D3, 0],
        [0, 0, 1, 0, D4],
        [0, 0, 0, 1, 0],
    ]
    a = [1, D1, D1 * D2, D1 * D2 * D3, D1 * D2 * D3 * D4]
    K2 = 10
    QB = [[0] * K2 for _ in range(K2)]
    for i in range(5):
        for j in range(5):
            QB[i][5 + j] = QH[i][j]
            QB[5 + i][j] = QH[i][j]
    lam1u, _, rhou = leading_ratio(QB, a + a, K2)
    print(
        f"validation, unperturbed B': rho = {mp.nstr(rhou, 12)} "
        f"(must be 0.999713000...)"
    )
    print(
        f"Delta lambda1 = {mp.nstr(lam1 - lam1u, 8)} "
        f"(NOTE: 1.77e-4, not the 2.49e-4 previously printed)"
    )
    with open("quotient_Bpe.json", "w") as fh:
        json.dump({"dim": K, "sizes": sizes, "Q": Q}, fh)
    print("quotient matrix archived to quotient_Bpe.json")


if __name__ == "__main__":
    main()
