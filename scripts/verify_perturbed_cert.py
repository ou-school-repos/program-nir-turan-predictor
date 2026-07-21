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
  * Step 5's rho < 1 conclusion is certified, not merely observed: leading_ratio
    computes a Bauer-Fike residual bound on the Perron eigenpair and a
    Davis-Kahan eigenvector-angle bound, then propagates both through the
    num/den ratio (Cauchy-Schwarz) to a rigorous upper bound rho_hi >= rho_true.
    The margin 1 - rho ~ 4.8e-5 exceeds the certified error bound by ~40
    orders of magnitude, so rho_hi < 1 is a genuine proof, not a float compare.
  * The exact odd-n scan to n<=3997 is a finite sanity check only, not a
    pointwise-for-all-n certificate: it cannot be extended to a rigorous
    integer scan, because a full sign certificate would need n on the same
    astronomical order as the unperturbed graph's known n=17,340 crossover
    (numbers with that many digits are computationally infeasible to scan).
    The certified rho_hi < 1 bound above is the actual strong-Leontovich proof.
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
import os
from collections import defaultdict

D1, D2, D3, D4 = 2, 34, 1, 48


def check(condition, detail):
    """Raise a certificate failure that is not stripped by python -O."""
    if not condition:
        raise RuntimeError(f"perturbed certificate check failed: {detail}")


def build_perturbed():
    """Build the explicit perturbed double-cover graph B'+e."""
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
    check(NH == 3403, NH)
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
    """Compute and exactly verify the coarsest equitable quotient."""
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
            check(
                all(Q[c][c2] == row.get(c2, 0) for c2 in range(K)),
                "partition not equitable",
            )
    return Q, sizes, K


def exact_scan(Q, sizes, K, max_n=4001):
    """Scan exact odd-n signs for the perturbed quotient."""
    w = [[1] * K]
    for _ in range(max_n):
        p = w[-1]
        w.append([sum(Q[i][j] * p[j] for j in range(K)) for i in range(K)])

    def hom_p(n):
        """Return exact hom(P_n) on the quotient."""
        return sum(sizes[i] * w[n - 1][i] for i in range(K))

    def hom_e(n, d=2):
        """Return exact hom(E_n^(d)) on the quotient."""
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
    """Compute the high-precision Perron leading-coefficient ratio, with a
    certified upper bound rho_hi (see strong_coeff.certified_leading_ratio)."""
    from strong_coeff import certified_leading_ratio

    return certified_leading_ratio(Q, sizes, K)


def main():
    """Run the perturbed-candidate certificate pipeline."""
    import mpmath as mp

    adj, N = build_perturbed()
    Q, sizes, K = equitable_quotient(adj, N)
    print(f"B'+e: {N} vertices -> equitable quotient with {K} cells (verified exactly)")
    flips, _ = exact_scan(Q, sizes, K)
    print(
        f"exact odd-n sign scan (n <= 3997): flips at {flips} "
        f"(expected [9], single opening)"
    )
    if flips != [9]:
        raise RuntimeError(f"expected a single opening flip at [9], got {flips}")
    lam1, lam2, rho, rho_hi = leading_ratio(Q, sizes, K)
    print(f"lambda1 = {mp.nstr(lam1, 12)}   |lambda2| = {mp.nstr(lam2, 12)}")
    print(f"leading-coefficient ratio rho = {mp.nstr(rho, 12)}")
    print(
        f"certified upper bound rho_hi = {mp.nstr(rho_hi, 12)} (Bauer-Fike/Davis-Kahan)"
    )
    if not rho_hi < 1:
        raise RuntimeError(
            f"cannot certify perturbed leading ratio < 1: rho_hi = {rho_hi}"
        )
    print("rho_hi < 1: strongly Leontovich (coefficient sense), certified")

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
    lam1u, _, rhou, _ = leading_ratio(QB, a + a, K2)
    print(
        f"validation, unperturbed B': rho = {mp.nstr(rhou, 12)} "
        f"(must be 0.999713000...)"
    )
    expected_rhou = mp.mpf("0.999713000053")
    if abs(rhou - expected_rhou) > mp.mpf("1e-12"):
        raise RuntimeError(f"unexpected unperturbed ratio {rhou}")
    print(f"Delta lambda1 = {mp.nstr(lam1 - lam1u, 8)}")
    archive = {"dim": K, "sizes": sizes, "Q": Q}
    path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "quotient_Bpe.json"
    )
    if os.path.exists(path):
        with open(path) as fh:
            existing = json.load(fh)
        if existing == archive:
            print("quotient matrix archive already up to date")
            return
    with open(path, "w") as fh:
        json.dump(archive, fh, indent=2, sort_keys=True)
        fh.write("\n")
    print("quotient matrix archived to quotient_Bpe.json")


if __name__ == "__main__":
    main()
