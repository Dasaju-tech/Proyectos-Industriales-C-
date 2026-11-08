"""Contract tests for the JSON-to-C++ generator; no third-party packages."""

import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "generate_model.py"
SPEC = importlib.util.spec_from_file_location("generate_model", SCRIPT)
generator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(generator)


class CodegenTests(unittest.TestCase):
    def setUp(self):
        self.model = generator.read_model(ROOT / "model" / "motor.json")

    def reject(self, model):
        with self.assertRaises(generator.ModelError):
            generator.render(model)

    def test_saved_header_matches_model(self):
        expected = generator.render(self.model).encode("ascii")
        self.assertEqual((ROOT / "INFO2_P1" / "MotorModel_generated.h").read_bytes(), expected)

    def test_forward_references_are_supported(self):
        self.model["nodes"].reverse()
        nodes, edges, outputs = generator.validate_model(self.model)
        self.assertEqual(len(nodes), 15)
        self.assertGreater(len(edges), 0)
        self.assertEqual(nodes[outputs["vo"]]["id"], "next_vo")
        self.assertIn("MotorModel_Evaluate", generator.render(self.model))

    def test_runtime_graph_algorithm_is_emitted(self):
        self.assertIn("DirectedGraph::TopologicalOrder", generator.render(self.model))

    def test_cycle_rejected(self):
        self.model["nodes"][7]["args"] = ["next_vo", "vo"]
        self.reject(self.model)

    def test_self_cycle_rejected(self):
        self.model["nodes"][7]["args"] = ["voltage_gap", "vo"]
        self.reject(self.model)

    def test_unknown_dependency_rejected(self):
        self.model["nodes"][7]["args"] = ["missing", "vo"]
        self.reject(self.model)

    def test_duplicate_ids_rejected(self):
        self.model["nodes"].append(copy.deepcopy(self.model["nodes"][0]))
        self.reject(self.model)

    def test_duplicate_json_keys_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "duplicate.json"
            path.write_text('{"nodes": [], "nodes": [], "outputs": {}}', encoding="utf-8")
            with self.assertRaises(generator.ModelError):
                generator.read_model(path)

    def test_unknown_operation_rejected(self):
        self.model["nodes"][7]["op"] = "system"
        self.reject(self.model)

    def test_invalid_constant_rejected(self):
        for value in (float("nan"), float("inf"), -float("inf"), True, "1", 10 ** 400):
            with self.subTest(value_type=type(value).__name__):
                self.model["nodes"][6]["value"] = value
                self.reject(self.model)

    def test_unknown_input_rejected(self):
        self.model["nodes"][0]["name"] = "sample_seconds_typo"
        self.reject(self.model)

    def test_invalid_binary_arity_rejected(self):
        self.model["nodes"][7]["args"] = ["vi"]
        self.reject(self.model)

    def test_invalid_output_rejected(self):
        self.model["outputs"]["vo"] = "missing"
        self.reject(self.model)

    def test_missing_output_rejected(self):
        del self.model["outputs"]["pos"]
        self.reject(self.model)

    def test_unknown_fields_rejected(self):
        self.model["nodes"][0]["typo"] = 1
        self.reject(self.model)

    def test_invalid_identifier_rejected(self):
        self.model["nodes"][0]["id"] = "x; evil()"
        self.reject(self.model)

    def test_oversize_graph_rejected(self):
        self.model["nodes"] = [{"id": "n%d" % i, "op": "constant", "value": 1}
                               for i in range(65)]
        self.reject(self.model)

    def test_cli_generation_and_stale_check(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "model.h"
            command = [sys.executable, str(SCRIPT), "--output", str(output)]
            subprocess.run(command, check=True, capture_output=True)
            first = output.read_bytes()
            subprocess.run(command, check=True, capture_output=True)
            self.assertEqual(output.read_bytes(), first)
            subprocess.run(command + ["--check"], check=True, capture_output=True)
            output.write_bytes(b"stale\n")
            result = subprocess.run(command + ["--check"], capture_output=True)
            self.assertEqual(result.returncode, 1)
            self.assertEqual(output.read_bytes(), b"stale\n")

    def test_invalid_model_does_not_overwrite_output(self):
        with tempfile.TemporaryDirectory() as directory:
            model = Path(directory) / "bad.json"
            output = Path(directory) / "kept.h"
            self.model["outputs"]["vo"] = "missing"
            model.write_text(json.dumps(self.model), encoding="utf-8")
            output.write_bytes(b"kept\n")
            result = subprocess.run([sys.executable, str(SCRIPT), "--model", str(model),
                                     "--output", str(output)], capture_output=True)
            self.assertEqual(result.returncode, 2)
            self.assertEqual(output.read_bytes(), b"kept\n")


if __name__ == "__main__":
    unittest.main()
