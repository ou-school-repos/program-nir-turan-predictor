# Exact Leading-Coefficient Certificates

`scripts/exact_rho.py` certifies the sign of `rho_d(H) - 1` for an
irreducible nonnegative integer quotient matrix. Unlike the high-precision
residual bound in `scripts/strong_coeff.py`, this path uses only integer
polynomials and rational root-isolating intervals.

## Method

For the quotient matrix `Q`, the verifier:

1. computes `chi(lambda) = det(lambda I - Q)` over the integers;
2. isolates the largest real root in a rational interval;
3. takes a nonzero column of `adj(lambda I - Q)` as a polynomial Perron
   eigenvector;
4. forms the exact numerator and denominator of `rho_d - 1`; and
5. refines the interval until neither sign polynomial has a root in it.

The final comparison is an exact algebraic sign certificate. The implementation
rejects reducible quotients and nonsimple Perron roots rather than applying the
argument outside its assumptions.

## Published Five-Orbit Audit

| Branching degrees | Exact conclusion |
| ----------------- | ---------------: |
| `(1,28,1,36)`     |      `rho_2 > 1` |
| `(1,16,1,21)`     |      `rho_2 > 1` |
| `(1,13,1,18)`     |      `rho_2 > 1` |
| `(1,12,1,18)`     |      `rho_2 > 1` |
| `(1,17,1,30)`     |      `rho_2 > 1` |
| `(1,15,1,26)`     |      `rho_2 > 1` |
| `(1,35,1,50)`     |      `rho_2 < 1` |
| `(2,34,1,48)`     |      `rho_2 < 1` |

Run the complete audit:

```bash
python3 -m unittest tests/test_exact_rho.py
```

Run one certificate:

```bash
python3 scripts/exact_rho.py 1 35 1 50 --depth 2
```

The command also accepts an arbitrary verified equitable quotient archive with
the keys `Q`, `sizes`, and optional `dim`. The verifier checks irreducibility,
nonnegative integral entries, and the undirected edge-balance identities
`s_i Q_ij = s_j Q_ji` before applying the certificate:

```bash
python3 scripts/exact_rho.py --input data/quotient_Bpe.json --depth 2 --json
python3 scripts/exact_rho.py --input quotient.json --output certificate.json
```

JSON output uses the versioned schema `exact-rho-certificate-v1` and records
the characteristic polynomial, rational Perron interval, sign polynomial,
denominator polynomial, depth, and certified relation. This is suitable for
archiving exact sweep results rather than retaining only floating-point ratios.

`scripts/sweep_strong_frontier.py` uses NumPy only as a prescreen. Every
reported hit is now accepted through `certify_rho_sign`, and its output archive
contains the quotient data and full exact certificate alongside the approximate
ratio used for ranking.

This prototype certifies named quotient matrices. It does not by itself upgrade
the historical `m <= 11` graph sweep: that requires retained quotient data or
a rerun that emits compact certificates for every target.
