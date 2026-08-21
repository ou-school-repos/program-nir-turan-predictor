#!/usr/bin/env python3
"""Tests for the spectral-prescreen certificate handoff."""

import io
import json
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

from sweep_orchestrator import certify_candidate, process_stream  # noqa: E402


class SweepOrchestratorTests(unittest.TestCase):
    """Check validation, parity routing, and deterministic archives."""

    def test_bipartite_candidate_is_deferred(self):
        """The path on three vertices is routed to parity analysis."""
        record = certify_candidate(
            {"type": "spectral_candidate", "g6": "Bg", "d": 2, "rho_estimate": 1.0}
        )
        self.assertEqual(record["status"], "deferred_bipartite_parity")
        self.assertNotIn("certificate", record)

    def test_nonbipartite_candidate_is_certified(self):
        """The triangle receives an exact algebraic certificate."""
        record = certify_candidate(
            {"type": "spectral_candidate", "g6": "Bw", "d": 2, "rho_estimate": 1.0}
        )
        self.assertEqual(record["status"], "exact_equal_one")
        self.assertEqual(record["certificate"]["relation"], "=")

    def test_stream_output_is_deterministic_ndjson(self):
        """Equivalent runs produce byte-identical compact JSON records."""
        candidate = json.dumps(
            {"type": "spectral_candidate", "g6": "Bw", "d": 2, "rho_estimate": 1.0}
        )
        first = io.StringIO()
        second = io.StringIO()
        self.assertEqual(process_stream([candidate], first), 1)
        self.assertEqual(process_stream([candidate], second), 1)
        self.assertEqual(first.getvalue(), second.getvalue())

    def test_invalid_record_reports_line_number(self):
        """Malformed protocol input fails closed with its source line."""
        with self.assertRaisesRegex(ValueError, "line 2"):
            process_stream(["\n", "{}\n"], io.StringIO())

    @patch("sweep_orchestrator.subprocess.run")
    def test_strict_arb_result_skips_symbolic_fallback(self, run):
        """A rigorous Arb separation is accepted without invoking SymPy."""
        run.return_value.returncode = 0
        run.return_value.stdout = json.dumps(
            {
                "schema": "arb-rho-certificate-v1",
                "status": "certified_above_one",
                "depth": 2,
                "precision_bits": 64,
                "rho_interval": "[1.03 +/- 0.001]",
            }
        )
        run.return_value.stderr = ""
        record = certify_candidate(
            {"type": "spectral_candidate", "g6": "C{", "d": 2, "rho_estimate": 1.03},
            Path(sys.executable),
        )
        self.assertEqual(record["status"], "certified_above_one")
        self.assertNotIn("certificate", record)
        self.assertTrue(Path(run.call_args.args[0][0]).is_absolute())


if __name__ == "__main__":
    unittest.main()
