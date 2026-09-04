"""High-precision leading-coefficient estimates for symmetric near-path targets.

This module uses mpmath plus residual/gap heuristics to produce numerical
estimates and conservative-looking enclosures for the Perron ratio. These
values are useful screening data, but they are not interval-arithmetic or
exact certificates. Exact sign claims belong to scripts/exact_rho.py.
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


def certified_ratio_bounds(Q, sizes, K, dps=50, depth=2):
    """Compute residual-based lower and upper estimates on the leading ratio.

    Returns (lam1, lam2, rho, rho_lo, rho_hi). The interval is derived from the
    computed eigenpair, its residual, and the observed spectral gap. It is a
    high-precision numerical estimate, not an outward-rounded proof object.
    """
    import mpmath as mp

    if depth < 1:
        raise ValueError("depth must be positive")
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
    wd = one
    for _ in range(depth):
        wd = [sum(Q[i][j] * wd[j] for j in range(K)) for i in range(K)]
    num = sum(sizes[i] * w1[i] * wd[i] * u1[i] for i in range(K))
    den = lam1 ** (depth + 1) * sum(sizes[i] * u1[i] for i in range(K))
    lam2 = max(abs(E[i]) for i in range(K - 1))

    # Residual/gap heuristic: use the symmetric similarity B and the computed
    # eigendata to estimate how much the dominant pair can drift numerically.
    Bv1 = [sum(B[i, j] * v1[j] for j in range(K)) for i in range(K)]
    resid = [Bv1[i] - lam1 * v1[i] for i in range(K)]
    resid_norm = mp.sqrt(sum(x**2 for x in resid))
    gap = lam1 - E[K - 2]
    if not gap > 2 * resid_norm:
        raise RuntimeError("spectral gap too small for a stable Perron estimate")
    eig_err = resid_norm
    # Use the residual-to-gap ratio as a conservative numerical proxy for the
    # eigenvector drift in this well-separated rank-1 setting.
    vec_err = 2 * resid_norm / gap

    c_num = [mp.sqrt(S[i]) * w1[i] * wd[i] for i in range(K)]
    c_den = [mp.sqrt(S[i]) for i in range(K)]
    norm_c_num = mp.sqrt(sum(x**2 for x in c_num))
    norm_c_den = mp.sqrt(sum(x**2 for x in c_den))
    err_num = norm_c_num * vec_err
    s_den = sum(c_den[i] * v1[i] for i in range(K))
    err_s_den = norm_c_den * vec_err

    num_hi = num + err_num
    num_lo = num - err_num
    lam1_hi = lam1 + eig_err
    lam1_lo = lam1 - eig_err
    s_den_hi = s_den + err_s_den
    s_den_lo = s_den - err_s_den
    if not (lam1_lo > 0 and s_den_lo > 0 and num_lo > 0):
        raise RuntimeError("eigenpair error estimate too loose for a stable ratio")
    den_hi = lam1_hi ** (depth + 1) * s_den_hi
    den_lo = lam1_lo ** (depth + 1) * s_den_lo
    rho_lo = num_lo / den_hi
    rho_hi = num_hi / den_lo

    return lam1, lam2, num / den, rho_lo, rho_hi


def certified_leading_ratio(Q, sizes, K, dps=50, depth=2):
    """Compute the high-precision Perron leading-coefficient ratio.

    Returns (lam1, lam2, rho, rho_hi), where rho_hi is the upper side of the
    residual-based numerical enclosure from certified_ratio_bounds.
    """
    lam1, lam2, rho, _rho_lo, rho_hi = certified_ratio_bounds(
        Q, sizes, K, dps=dps, depth=depth
    )
    return lam1, lam2, rho, rho_hi


def leading_ratio(degrees):
    """Return the upper side of the residual-based depth-2 ratio estimate.

    This is useful as a high-precision screen, but exact sign claims should use
    scripts/exact_rho.py rather than treating rho_hi as a formal certificate.
    """
    q, sizes = quotient_data(degrees)
    _, _, _, rho_hi = certified_leading_ratio(q, sizes, len(q))
    return rho_hi
