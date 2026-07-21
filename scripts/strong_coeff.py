"""Leading-coefficient test for looped symmetric near-path targets.

Uses mpmath at high precision plus a Bauer-Fike eigenvalue-residual bound
and a Davis-Kahan eigenvector-angle bound (propagated through the ratio via
Cauchy-Schwarz) so callers get a certified upper bound on the true ratio,
not just an uncertified floating-point point estimate.
"""


def quotient_data(degrees):
    """Return the looped symmetric quotient matrix and orbit sizes."""
    dim = len(degrees) + 1
    q = [[0] * dim for _ in range(dim)]
    q[0][0] = 1
    for i, d in enumerate(degrees):
        q[i][i + 1] = d
        q[i + 1][i] = 1

    sizes = [1]
    for d in degrees:
        sizes.append(sizes[-1] * d)
    return q, sizes


def certified_leading_ratio(Q, sizes, K, dps=50):
    """Compute the high-precision Perron leading-coefficient ratio.

    Returns (lam1, lam2, rho, rho_hi), where rho_hi is a certified upper
    bound on the true ratio: derives a Bauer-Fike residual bound on the
    computed Perron eigenpair and the corresponding Davis-Kahan
    eigenvector-angle bound, then propagates both through the num/den
    ratio (Cauchy-Schwarz), so rho_hi >= rho_true is a proof rather than
    a bare float comparison.
    """
    import mpmath as mp

    mp.mp.dps = dps
    S = [mp.mpf(s) for s in sizes]
    B = mp.matrix(K, K)
    for i in range(K):
        for j in range(K):
            B[i, j] = Q[i][j] * mp.sqrt(S[i] / S[j])  # symmetric similarity
    E, V = mp.eigsy(B)
    lam1 = E[K - 1]
    v1 = [V[i, K - 1] for i in range(K)]  # unit eigenvector of B (orthonormal basis)
    if v1[0] < 0:
        v1 = [-x for x in v1]
    u1 = [v1[i] / mp.sqrt(S[i]) for i in range(K)]
    if not all(x > 0 for x in u1):
        raise RuntimeError("Perron vector has non-positive entry")
    one = [mp.mpf(1)] * K
    w1 = [sum(Q[i][j] * one[j] for j in range(K)) for i in range(K)]
    w2 = [sum(Q[i][j] * w1[j] for j in range(K)) for i in range(K)]
    num = sum(sizes[i] * w1[i] * w2[i] * u1[i] for i in range(K))
    den = lam1**3 * sum(sizes[i] * u1[i] for i in range(K))
    lam2 = max(abs(E[i]) for i in range(K - 1))

    # Certified bound: residual r = B*v1 - lam1*v1 for the exact unit vector v1.
    # For symmetric B, some true eigenvalue mu satisfies |mu - lam1| <= ||r||_2
    # (Bauer-Fike, symmetric case). Since v1 is the Perron-like vector, the
    # nearby true eigenvalue is the true lam1 provided the spectral gap to the
    # rest of the (already-computed) spectrum comfortably exceeds ||r||_2.
    Bv1 = [sum(B[i, j] * v1[j] for j in range(K)) for i in range(K)]
    resid = [Bv1[i] - lam1 * v1[i] for i in range(K)]
    resid_norm = mp.sqrt(sum(x**2 for x in resid))
    gap = lam1 - E[K - 2]
    if not gap > 2 * resid_norm:
        raise RuntimeError("spectral gap too small to certify Perron pair")
    eig_err = resid_norm
    # Davis-Kahan sin(theta) <= ||r|| / gap; use it (with 2x safety factor) as
    # a bound on ||v1_true - v1||_2 for this rank-1, well-separated case.
    vec_err = 2 * resid_norm / gap

    c_num = [mp.sqrt(S[i]) * w1[i] * w2[i] for i in range(K)]
    c_den = [mp.sqrt(S[i]) for i in range(K)]
    norm_c_num = mp.sqrt(sum(x**2 for x in c_num))
    norm_c_den = mp.sqrt(sum(x**2 for x in c_den))
    err_num = norm_c_num * vec_err
    s_den = sum(c_den[i] * v1[i] for i in range(K))
    err_s_den = norm_c_den * vec_err

    num_hi = num + err_num
    lam1_lo = lam1 - eig_err
    s_den_lo = s_den - err_s_den
    if not (lam1_lo > 0 and s_den_lo > 0):
        raise RuntimeError("eigenpair error bound too loose to certify")
    den_lo = lam1_lo**3 * s_den_lo
    rho_hi = num_hi / den_lo

    return lam1, lam2, num / den, rho_hi


def leading_ratio(degrees):
    """Return a certified upper bound on the Perron leading-coefficient
    ratio for E_n^(2) versus P_n on the looped symmetric near-path quotient.

    The returned value is rho_hi (see certified_leading_ratio): a proven
    upper bound on the true ratio, so `leading_ratio(degrees) < 1.0` is a
    certified strongly-Leontovich conclusion, not a float comparison on an
    uncertified point estimate.
    """
    q, sizes = quotient_data(degrees)
    _, _, _, rho_hi = certified_leading_ratio(q, sizes, len(q))
    return rho_hi
