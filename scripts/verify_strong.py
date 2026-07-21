#!/usr/bin/env python3
"""
Verify Leontovich vs. STRONGLY Leontovich for looped 5-orbit trees T^(d1,d2,d3,d4).

Distinction (this is the fix):
  * Leontovich          : Hom(E_n) < Hom(P_n) for SOME odd n  (a crossover exists)
  * strongly Leontovich : Hom(E_n) < Hom(P_n) for ALL large odd n
                          <=> E has the strictly smaller leading coefficient
                          <=> Delta = Hom(P_n) - Hom(E_n) stays > 0 as n -> infinity.

The original verify_strong.py scanned only n<=50 and printed "STRONG LEONTOVICH"
for any single Delta>0. That cannot see a SECOND crossover where P overtakes E
again, so finite-window (ordinary) graphs were mislabeled as strong.
"""

from strong_coeff import leading_ratio


def analyze(d1, d2, d3, d4, max_n=17501):
    """Analyze finite crossovers and coefficient strongness for one tree."""
    params = (d1, d2, d3, d4)
    Q = [
        [1, d1, 0, 0, 0],
        [1, 0, d2, 0, 0],
        [0, 1, 0, d3, 0],
        [0, 0, 1, 0, d4],
        [0, 0, 0, 1, 0],
    ]
    a = [1, d1, d1 * d2, d1 * d2 * d3, d1 * d2 * d3 * d4]
    V = sum(a)

    # exact integer walk vectors w[k] = Q^k * 1
    w = [[1, 1, 1, 1, 1]]
    for _ in range(max_n):
        p = w[-1]
        w.append([sum(Q[i][j] * p[j] for j in range(5)) for i in range(5)])

    def homP(n):
        """Return exact hom(P_n) for the current tree quotient."""
        return sum(a[i] * w[n - 1][i] for i in range(5))

    def homE(n, d=2):  # E_n^(d), pendant at depth d
        """Return exact hom(E_n^(d)) for the current tree quotient."""
        stem = n - d - 2
        return sum(a[i] * w[stem][i] * w[1][i] * w[d][i] for i in range(5))

    # sign of Delta = homP - homE over odd n; a strong graph keeps Delta>0 forever
    flips, prev = [], None
    saw_positive = False
    for n in range(5, max_n - 3, 2):
        s = 1 if homP(n) - homE(n) > 0 else -1  # +1 == E beats P as minimizer
        saw_positive |= s == 1
        if prev is not None and s != prev:
            flips.append(n)
        prev = s
    last_odd = max_n if max_n % 2 == 1 else max_n - 1
    rho = float(leading_ratio(params))

    leontovich = saw_positive
    strong = leontovich and rho < 1.0
    return V, flips, leontovich, strong, last_odd, rho


def report(params):
    """Print one human-readable strongness audit row."""
    V, flips, leo, strong, last_odd, rho = analyze(*params)
    verdict = (
        "STRONGLY Leontovich"
        if strong
        else "Leontovich (finite window only)" if leo else "not Leontovich"
    )
    win = "" if not flips else f"  window opens at n={flips[0]}"
    if len(flips) >= 2:
        win += f", closes at n={flips[1]}"
        if not strong:
            win += " (P wins again -> NOT strong)"
    print(
        f"T^{params}: |V|={V:5d} | crossovers at odd n={flips} "
        f"| rho={rho:.12f} | scanned odd n<= {last_odd} | {verdict}{win}"
    )


if __name__ == "__main__":
    print("=== Strong-Leontovich audit of the paper's Table 8 frontier ===\n")
    for p in [
        (1, 28, 1, 36),
        (1, 16, 1, 21),
        (1, 13, 1, 18),
        (1, 12, 1, 18),
        (1, 17, 1, 30),
        (1, 15, 1, 26),
    ]:
        report(p)
    print("\n=== Graphs the double-cover section relies on ===\n")
    for p in [(1, 35, 1, 50), (2, 34, 1, 48)]:
        report(p)
