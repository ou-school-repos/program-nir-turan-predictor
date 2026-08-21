#!/usr/bin/env python3
"""Tests for exact algebraic leading-ratio certificates."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

from exact_rho import certify_rho_sign, symmetric_tree_quotient  # noqa: E402


class ExactRhoTests(unittest.TestCase):
    """Check exact signs against established frontier examples."""

    def certify(self, degrees):
        """Return the exact depth-2 sign for one radial tree."""
        quotient, sizes = symmetric_tree_quotient(degrees)
        return certify_rho_sign(quotient, sizes).sign

    def test_strong_witness_is_below_one(self):
        """The 1,822-vertex witness has rho_2 below one."""
        self.assertEqual(self.certify((1, 35, 1, 50)), -1)

    def test_finite_window_example_is_above_one(self):
        """A finite-window target eventually returns above one."""
        self.assertEqual(self.certify((1, 28, 1, 36)), 1)

    def test_published_frontier_signs(self):
        """Certify every published five-orbit frontier sign."""
        above = [
            (1, 28, 1, 36),
            (1, 16, 1, 21),
            (1, 13, 1, 18),
            (1, 12, 1, 18),
            (1, 17, 1, 30),
            (1, 15, 1, 26),
        ]
        below = [(1, 35, 1, 50), (2, 34, 1, 48)]
        for degrees in above:
            with self.subTest(degrees=degrees):
                self.assertEqual(self.certify(degrees), 1)
        for degrees in below:
            with self.subTest(degrees=degrees):
                self.assertEqual(self.certify(degrees), -1)

    def test_invalid_reducible_quotient_is_rejected(self):
        """Perron certification requires an irreducible quotient."""
        with self.assertRaises(ValueError):
            certify_rho_sign([[1, 0], [0, 1]], [1, 1])


if __name__ == "__main__":
    unittest.main()
