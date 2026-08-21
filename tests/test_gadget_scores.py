"""Regression tests for asymptotic gadget scores."""

import sys
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPTS))

from exact_rho import certify_rho_sign, symmetric_tree_quotient  # noqa: E402
from gadget_scores import long_spider_score, near_path_ratio  # noqa: E402


class GadgetScoreTests(unittest.TestCase):
    """Check the named strongly Leontovich radial targets."""

    def test_smaller_witness_depth_four_is_best_reported_depth(self):
        quotient, sizes = symmetric_tree_quotient((1, 35, 1, 50))
        expected = {
            2: 0.999953714414,
            4: 0.999932644624,
            6: 1.000314977232,
            8: 1.000997907982,
        }
        for depth, value in expected.items():
            self.assertAlmostEqual(near_path_ratio(quotient, sizes, depth), value, 12)
        self.assertEqual(certify_rho_sign(quotient, sizes, 4).relation, "<")
        self.assertLess(
            near_path_ratio(quotient, sizes, 4),
            near_path_ratio(quotient, sizes, 2),
        )

    def test_long_spider_scores_exceed_one(self):
        cases = {
            (1, 35, 1, 50): 2.416875686287,
            (2, 34, 1, 48): 2.446428126638,
        }
        for degrees, expected in cases.items():
            quotient, sizes = symmetric_tree_quotient(degrees)
            self.assertAlmostEqual(long_spider_score(quotient, sizes), expected, 12)
            self.assertGreaterEqual(long_spider_score(quotient, sizes), 1.0)


if __name__ == "__main__":
    unittest.main()
