"""Exercise the compiled simulator through its public, deterministic CSV CLI."""

import csv
import io
import math
import os
from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[1]
BINARY = Path(os.environ.get("MOTOR_BINARY", str(ROOT / "build" / "motor")))


class SimulatorCliTests(unittest.TestCase):
    def run_motor(self, *arguments):
        return subprocess.run(
            [str(BINARY), *map(str, arguments)],
            cwd=ROOT,
            text=True,
            capture_output=True,
            timeout=20,
            check=False,
        )

    def simulate(self, *arguments):
        result = self.run_motor("--simulate", *arguments)
        self.assertEqual(result.returncode, 0, result.stderr)
        reader = csv.DictReader(io.StringIO(result.stdout))
        self.assertEqual(
            reader.fieldnames,
            ["step", "time_s", "vi_v", "vo_v", "vel_rpm", "pos_deg"],
        )
        return list(reader)

    def test_default_step_response_matches_closed_form(self):
        rows = self.simulate(20)
        self.assertEqual(len(rows), 21)
        expected_pos = 0.0
        for step, row in enumerate(rows):
            expected_vo = 1.0 - 0.9**step
            expected_vel = 100.0 * expected_vo
            if step:
                expected_pos += expected_vel * 6.0
            self.assertEqual(int(row["step"]), step)
            self.assertAlmostEqual(float(row["time_s"]), step, places=8)
            self.assertEqual(float(row["vi_v"]), 1.0)
            self.assertAlmostEqual(float(row["vo_v"]), expected_vo, places=8)
            self.assertAlmostEqual(float(row["vel_rpm"]), expected_vel, places=7)
            self.assertAlmostEqual(float(row["pos_deg"]), expected_pos, places=5)

    def test_negative_voltage_and_millisecond_conversion(self):
        rows = self.simulate(30, -2, 2, 50, 100)
        self.assertEqual(len(rows), 31)
        for step, row in enumerate(rows):
            self.assertAlmostEqual(float(row["time_s"]), step * 0.1, places=8)
            self.assertAlmostEqual(
                float(row["vel_rpm"]), -100 * (1 - 0.95**step), places=7
            )
        self.assertLess(float(rows[-1]["pos_deg"]), 0)

    def test_convergence_to_static_gain(self):
        final = self.simulate(400, 3, 10, 100, 1000)[-1]
        self.assertAlmostEqual(float(final["vel_rpm"]), 300, places=7)
        self.assertTrue(all(math.isfinite(float(value)) for value in final.values()))

    def test_dt_equal_to_tau(self):
        rows = self.simulate(1, 4, 0.25, 7, 250)
        self.assertEqual(float(rows[1]["vo_v"]), 4)
        self.assertEqual(float(rows[1]["vel_rpm"]), 28)
        self.assertEqual(float(rows[1]["pos_deg"]), 42)

    def test_invalid_arguments_fail(self):
        cases = [
            ("--simulate",),
            ("--simulate", "0"),
            ("--simulate", "-1"),
            ("--simulate", "1.5"),
            ("--simulate", "1000001"),
            ("--simulate", "1junk"),
            ("--simulate", "1", "nan"),
            ("--simulate", "1", "inf"),
            ("--simulate", "1", "1e999"),
            ("--simulate", "1", "1", "0"),
            ("--simulate", "1", "1", "10", "-1"),
            ("--simulate", "1", "1", "10", "100", "0"),
            ("--simulate", "1", "1", "1", "100", "1001"),
            ("--simulate", "1", "1", "10", "100", "1000", "extra"),
            ("--simulate", "1000000", "0", "1e308", "0", "1e308"),
            ("--unknown",),
        ]
        for arguments in cases:
            with self.subTest(arguments=arguments):
                result = self.run_motor(*arguments)
                self.assertNotEqual(result.returncode, 0)
                self.assertTrue(result.stderr.strip(), result.stdout)

    def test_numerical_overflow_is_reported(self):
        result = self.run_motor("--simulate", 1, "1e308", 10, "1e308", 1000)
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(result.stderr.strip())

    def test_help_is_available_without_simulating(self):
        result = self.run_motor("--help")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("--simulate", result.stdout)


if __name__ == "__main__":
    unittest.main()
