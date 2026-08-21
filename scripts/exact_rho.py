#!/usr/bin/env python3
"""Certify the sign of the Perron leading-coefficient ratio minus one.

For an irreducible nonnegative integer quotient matrix Q, this module works
entirely over ZZ and QQ:

1. isolate the Perron root of the characteristic polynomial;
2. obtain a polynomial right eigenvector from adj(lambda I - Q);
3. form the numerator and denominator of rho_d - 1; and
4. refine the rational isolating interval until both signs are constant.

The resulting sign is exact. No floating-point eigenvalue or tolerance enters
the certificate.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from typing import Iterable

import sympy as sp


@dataclass(frozen=True)
class RhoCertificate:
    """Exact sign certificate for a leading-coefficient ratio."""

    characteristic_polynomial: sp.Poly
    perron_interval: tuple[sp.Rational, sp.Rational]
    sign_polynomial: sp.Poly
    denominator_polynomial: sp.Poly
    sign: int
    depth: int

    @property
    def relation(self) -> str:
        """Return the certified comparison between rho_d and one."""
        return "<" if self.sign < 0 else ">" if self.sign > 0 else "="


def symmetric_tree_quotient(
    degrees: Iterable[int],
) -> tuple[list[list[int]], list[int]]:
    """Return the integer quotient and orbit sizes for a looped radial tree."""
    degrees = tuple(degrees)
    if not degrees or any(d <= 0 for d in degrees):
        raise ValueError("branching degrees must be positive")
    dim = len(degrees) + 1
    quotient = [[0] * dim for _ in range(dim)]
    quotient[0][0] = 1
    for i, degree in enumerate(degrees):
        quotient[i][i + 1] = degree
        quotient[i + 1][i] = 1
    sizes = [1]
    for degree in degrees:
        sizes.append(sizes[-1] * degree)
    return quotient, sizes


def _validate_data(quotient: list[list[int]], sizes: list[int]) -> sp.Matrix:
    """Validate an irreducible nonnegative integer quotient."""
    dim = len(quotient)
    if dim == 0 or len(sizes) != dim or any(len(row) != dim for row in quotient):
        raise ValueError("quotient must be square and match the size vector")
    if any(size <= 0 for size in sizes):
        raise ValueError("orbit sizes must be positive")
    if any(not isinstance(x, int) or x < 0 for row in quotient for x in row):
        raise ValueError("quotient entries must be nonnegative integers")

    reachable = {0}
    pending = [0]
    while pending:
        i = pending.pop()
        for j, value in enumerate(quotient[i]):
            if value and j not in reachable:
                reachable.add(j)
                pending.append(j)
    if len(reachable) != dim:
        raise ValueError("quotient must be irreducible")
    return sp.Matrix(quotient)


def _perron_interval(
    polynomial: sp.Poly, precision: int
) -> tuple[sp.Rational, sp.Rational]:
    """Return a rational interval isolating the largest real root."""
    intervals = polynomial.intervals(eps=sp.Rational(1, 10**precision))
    real = [(interval, multiplicity) for interval, multiplicity in intervals]
    if not real:
        raise ValueError("characteristic polynomial has no real root")
    interval, multiplicity = max(real, key=lambda item: item[0][1])
    if multiplicity != 1:
        raise ValueError("Perron root is not simple")
    left, right = map(sp.Rational, interval)
    if right <= 0:
        raise ValueError("Perron root is not positive")
    return left, right


def _sign_at_isolated_root(
    polynomial: sp.Poly,
    characteristic: sp.Poly,
    interval: tuple[sp.Rational, sp.Rational],
) -> int | None:
    """Return a polynomial's sign on an isolating interval, if determined."""
    left, right = interval
    common = sp.gcd(polynomial, characteristic)
    if common.degree() > 0 and common.count_roots(left, right):
        return 0
    if polynomial.count_roots(left, right):
        return None
    value = polynomial.eval((left + right) / 2)
    return 1 if value > 0 else -1 if value < 0 else None


def certify_rho_sign(
    quotient: list[list[int]],
    sizes: list[int],
    depth: int = 2,
    max_precision: int = 80,
) -> RhoCertificate:
    """Certify whether rho_depth is below, equal to, or above one."""
    if depth < 1:
        raise ValueError("depth must be positive")
    matrix = _validate_data(quotient, sizes)
    dim = matrix.rows
    lam = sp.Symbol("lambda")
    characteristic = sp.Poly(matrix.charpoly(lam).as_expr(), lam, domain=sp.ZZ)
    adjugate = lam * sp.eye(dim) - matrix
    adjugate = adjugate.adjugate()

    ones = sp.ones(dim, 1)
    w1 = matrix * ones
    wd = matrix**depth * ones
    weights = sp.Matrix(sizes)
    coefficient = sp.Matrix([sizes[i] * w1[i] * wd[i] for i in range(dim)])

    candidates: list[tuple[sp.Poly, sp.Poly]] = []
    for column in range(dim):
        eigenvector = adjugate[:, column]
        numerator = sp.expand((coefficient.T * eigenvector)[0])
        denominator = sp.expand(lam ** (depth + 1) * (weights.T * eigenvector)[0])
        if numerator != 0 and denominator != 0:
            candidates.append(
                (
                    sp.Poly(numerator - denominator, lam, domain=sp.ZZ),
                    sp.Poly(denominator, lam, domain=sp.ZZ),
                )
            )

    for precision in range(8, max_precision + 1, 8):
        interval = _perron_interval(characteristic, precision)
        for sign_polynomial, denominator_polynomial in candidates:
            numerator_sign = _sign_at_isolated_root(
                sign_polynomial, characteristic, interval
            )
            denominator_sign = _sign_at_isolated_root(
                denominator_polynomial, characteristic, interval
            )
            if numerator_sign is not None and denominator_sign not in (None, 0):
                sign = 0 if numerator_sign == 0 else numerator_sign * denominator_sign
                return RhoCertificate(
                    characteristic,
                    interval,
                    sign_polynomial,
                    denominator_polynomial,
                    sign,
                    depth,
                )
    raise RuntimeError(
        f"could not separate rho_{depth} from one at {max_precision} decimal digits"
    )


def main() -> None:
    """Certify one looped radial-tree parameter tuple from the command line."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "degrees", nargs="+", type=int, help="positive radial branching degrees"
    )
    parser.add_argument("--depth", type=int, default=2)
    args = parser.parse_args()
    quotient, sizes = symmetric_tree_quotient(args.degrees)
    certificate = certify_rho_sign(quotient, sizes, args.depth)
    left, right = certificate.perron_interval
    print(f"chi(lambda) = {certificate.characteristic_polynomial.as_expr()}")
    print(f"Perron root in [{left}, {right}]")
    print(f"rho_{args.depth} {certificate.relation} 1 (exact algebraic certificate)")


if __name__ == "__main__":
    main()
