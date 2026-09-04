#!/usr/bin/env python3
"""Tests for the spectral-prescreen certificate handoff."""

import io
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

from sweep_orchestrator import certify_candidate, main, process_stream  # noqa: E402


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

    def test_malformed_graph6_is_rejected(self):
        """Malformed graph6 input fails before certification starts."""
        with self.assertRaisesRegex(ValueError, "graph6 payload length"):
            certify_candidate(
                {"type": "spectral_candidate", "g6": "B", "d": 2, "rho_estimate": 1.0}
            )

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

    def test_same_input_and_output_path_is_rejected(self):
        """The CLI refuses to truncate its own input file."""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidates.ndjson"
            path.write_text(
                json.dumps(
                    {
                        "type": "spectral_candidate",
                        "g6": "Bg",
                        "d": 2,
                        "rho_estimate": 1.0,
                    }
                )
                + "\n",
                encoding="utf-8",
            )
            with patch.object(
                sys,
                "argv",
                [
                    "sweep_orchestrator.py",
                    "--input",
                    str(path),
                    "--output",
                    str(path),
                ],
            ):
                with self.assertRaises(SystemExit) as ctx:
                    main()
            self.assertEqual(ctx.exception.code, 2)

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
            {"type": "spectral_candidate", "g6": "Bw", "d": 2, "rho_estimate": 1.03},
            Path(sys.executable),
        )
        self.assertEqual(record["status"], "certified_above_one")
        self.assertNotIn("certificate", record)
        self.assertTrue(Path(run.call_args.args[0][0]).is_absolute())

    @patch("sweep_orchestrator.subprocess.run")
    def test_strict_arb_result_requires_precision_metadata(self, run):
        """Certified Arb results must include the precision metadata."""
        run.return_value.returncode = 0
        run.return_value.stdout = json.dumps(
            {
                "schema": "arb-rho-certificate-v1",
                "status": "certified_below_one",
                "depth": 2,
            }
        )
        run.return_value.stderr = ""
        with self.assertRaisesRegex(RuntimeError, "invalid precision_bits value"):
            certify_candidate(
                {
                    "type": "spectral_candidate",
                    "g6": "Bw",
                    "d": 2,
                    "rho_estimate": 0.97,
                },
                Path(sys.executable),
            )

    @patch("sweep_orchestrator.subprocess.run")
    def test_disconnected_arb_result_is_preserved(self, run):
        """Disconnected Arb candidates are routed through the known status."""
        run.return_value.returncode = 1
        run.return_value.stdout = json.dumps(
            {
                "schema": "arb-rho-certificate-v1",
                "status": "invalid_disconnected",
                "depth": 2,
            }
        )
        run.return_value.stderr = ""
        record = certify_candidate(
            {"type": "spectral_candidate", "g6": "Bg", "d": 2, "rho_estimate": 1.0},
            Path(sys.executable),
        )
        self.assertEqual(record["status"], "invalid_disconnected")

    @patch("sweep_orchestrator.certify_rho_sign")
    def test_symbolic_certification_failure_does_not_abort_stream(self, certify):
        """A symbolic failure is archived and later records continue."""
        certify.side_effect = RuntimeError("precision limit")
        source = [
            json.dumps(
                {
                    "type": "spectral_candidate",
                    "g6": "Bw",
                    "d": 2,
                    "rho_estimate": 1.0,
                }
            ),
            json.dumps(
                {
                    "type": "spectral_candidate",
                    "g6": "Bg",
                    "d": 2,
                    "rho_estimate": 1.0,
                }
            ),
        ]
        destination = io.StringIO()

        with self.assertLogs("sweep_orchestrator", level="WARNING") as logs:
            self.assertEqual(process_stream(source, destination), 2)
        records = [json.loads(line) for line in destination.getvalue().splitlines()]
        self.assertEqual(records[0]["status"], "unresolved")
        self.assertEqual(records[0]["certificate_error"], "precision limit")
        self.assertEqual(records[1]["status"], "deferred_bipartite_parity")
        self.assertIn("Exact rho certification unresolved", logs.output[0])


if __name__ == "__main__":
    unittest.main()
