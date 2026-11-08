#!/usr/bin/env python3
"""Validate a motor arithmetic DAG and emit deterministic C++98 (stdlib only)."""

import argparse
from collections import deque
import json
import math
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
MAX_NODES = 64
MAX_EDGES = 128
INPUTS = ("dt", "tau", "km", "vi", "vo", "pos")
BINARY = ("add", "subtract", "multiply", "divide")
OP_CODES = {"input": 0, "constant": 1, "add": 2, "subtract": 3,
            "multiply": 4, "divide": 5}


class ModelError(ValueError):
    """The model cannot be represented safely by the generated evaluator."""


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ModelError("duplicate JSON key: " + key)
        result[key] = value
    return result


def read_model(path):
    with Path(path).open(encoding="utf-8") as stream:
        return json.load(stream, object_pairs_hook=unique_object)


def validate_model(model):
    """Return (nodes, edges, output indices), accepting forward references."""
    if not isinstance(model, dict) or set(model) != {"nodes", "outputs"}:
        raise ModelError("model must contain exactly nodes and outputs")
    nodes = model["nodes"]
    if not isinstance(nodes, list) or not 1 <= len(nodes) <= MAX_NODES:
        raise ModelError("nodes must be a nonempty list of at most 64 nodes")
    indices = {}
    for index, node in enumerate(nodes):
        if not isinstance(node, dict):
            raise ModelError("each node must be an object")
        name = node.get("id")
        if not isinstance(name, str) or re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name) is None:
            raise ModelError("node id must be an ASCII identifier")
        if name in indices:
            raise ModelError("duplicate node id: " + name)
        indices[name] = index
    edges = []
    for index, node in enumerate(nodes):
        op = node.get("op")
        if not isinstance(op, str) or op not in OP_CODES:
            raise ModelError("unsupported operation for " + node["id"])
        if op == "input":
            if set(node) != {"id", "op", "name"} or node["name"] not in INPUTS:
                raise ModelError("input must name one of: " + ", ".join(INPUTS))
        elif op == "constant":
            if set(node) != {"id", "op", "value"}:
                raise ModelError("constant must contain exactly id, op and value")
            value = node["value"]
            try:
                valid = type(value) in (int, float) and math.isfinite(value)
            except OverflowError:
                valid = False
            if not valid:
                raise ModelError("constant must be a finite number")
        else:
            if set(node) != {"id", "op", "args"}:
                raise ModelError("binary node must contain exactly id, op and args")
            args = node["args"]
            if not isinstance(args, list) or len(args) != 2:
                raise ModelError("binary operation requires exactly two references")
            for dependency in args:
                if not isinstance(dependency, str) or dependency not in indices:
                    raise ModelError("unknown dependency for " + node["id"])
                edges.append((indices[dependency], index))
    if len(edges) > MAX_EDGES:
        raise ModelError("model exceeds 128 dependency edges")
    indegree = [0] * len(nodes)
    outgoing = [[] for _ in nodes]
    for source, target in edges:
        indegree[target] += 1
        outgoing[source].append(target)
    queue = deque(i for i, degree in enumerate(indegree) if degree == 0)
    visited = 0
    while queue:
        source = queue.popleft()
        visited += 1
        for target in outgoing[source]:
            indegree[target] -= 1
            if indegree[target] == 0:
                queue.append(target)
    if visited != len(nodes):
        raise ModelError("directed cycle in model")
    outputs = model["outputs"]
    if not isinstance(outputs, dict) or set(outputs) != {"vo", "vel", "pos"}:
        raise ModelError("outputs must contain exactly vo, vel and pos")
    if any(not isinstance(ref, str) or ref not in indices for ref in outputs.values()):
        raise ModelError("output references an unknown node")
    return nodes, edges, {name: indices[outputs[name]] for name in ("vo", "vel", "pos")}


def render(model):
    nodes, edges, outputs = validate_model(model)
    indices = {node["id"]: i for i, node in enumerate(nodes)}
    rows = []
    for node in nodes:
        op = node["op"]
        left = right = 0
        value = 0.0
        if op == "input":
            left = INPUTS.index(node["name"])
        elif op == "constant":
            value = float(node["value"])
        else:
            left, right = [indices[ref] for ref in node["args"]]
        number = format(value, ".17g")
        if "." not in number and "e" not in number:
            number += ".0"
        rows.append("        { %d, %d, %d, %s }, // %s" %
                    (OP_CODES[op], left, right, number, node["id"]))
    edge_rows = ["        { %d, %d }," % edge for edge in edges]
    # C++98 does not permit zero-length arrays; a sentinel is never consumed.
    if not edge_rows:
        edge_rows = ["        { 0, 0 }, // unused sentinel: no dependency edges"]
    return '''// Generated by tools/generate_model.py; edit model/motor.json instead.
// Deterministic arithmetic DAG; Kahn ordering is evaluated in C++ at runtime.
#ifndef MOTOR_MODEL_GENERATED_H
#define MOTOR_MODEL_GENERATED_H

#include "DirectedGraph.h"
#include "Numeric.h"

namespace MotorModel_Detail
{
    enum { NODE_COUNT = %d, EDGE_COUNT = %d };
    struct Node { int op; int left; int right; double constant; };
    static const Node NODES[NODE_COUNT] = {
%s
    };
    static const DirectedGraph::Edge EDGES[%d] = {
%s
    };
}

// Euler step with bounded stack storage. dt and tau are measured in seconds.
// Restrict dt <= tau to keep the first-order response monotonic.
// No outputs are changed if the graph or any numerical result is invalid.
inline bool MotorModel_Evaluate(double dt, double tau, double km,
                                double vi, double vo, double pos,
                                double* next_vo, double* next_vel, double* next_pos)
{
    if (next_vo == 0 || next_vel == 0 || next_pos == 0 ||
        next_vo == next_vel || next_vo == next_pos || next_vel == next_pos ||
        !Motor_IsFinite(dt) || !Motor_IsFinite(tau) || !Motor_IsFinite(km) ||
        !Motor_IsFinite(vi) || !Motor_IsFinite(vo) || !Motor_IsFinite(pos) ||
        dt <= 0.0 || tau <= 0.0 || dt > tau || km < 0.0)
        return false;
    int order[MotorModel_Detail::NODE_COUNT];
    double values[MotorModel_Detail::NODE_COUNT];
    const double inputs[6] = { dt, tau, km, vi, vo, pos };
    if (!DirectedGraph::TopologicalOrder(MotorModel_Detail::NODE_COUNT,
            MotorModel_Detail::EDGES, MotorModel_Detail::EDGE_COUNT, order))
        return false;
    for (int i = 0; i < MotorModel_Detail::NODE_COUNT; ++i)
    {
        const int index = order[i];
        const MotorModel_Detail::Node& node = MotorModel_Detail::NODES[index];
        double result;
        switch (node.op)
        {
        case 0: result = inputs[node.left]; break;
        case 1: result = node.constant; break;
        case 2: result = values[node.left] + values[node.right]; break;
        case 3: result = values[node.left] - values[node.right]; break;
        case 4: result = values[node.left] * values[node.right]; break;
        case 5:
            if (values[node.right] == 0.0) return false;
            result = values[node.left] / values[node.right];
            break;
        default: return false;
        }
        if (!Motor_IsFinite(result)) return false;
        values[index] = result;
    }
    *next_vo = values[%d];
    *next_vel = values[%d];
    *next_pos = values[%d];
    return true;
}

#endif
''' % (len(nodes), len(edges), "\n".join(rows), max(1, len(edges)),
       "\n".join(edge_rows), outputs["vo"], outputs["vel"], outputs["pos"])


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", type=Path, default=ROOT / "model" / "motor.json")
    parser.add_argument("--output", type=Path,
                        default=ROOT / "INFO2_P1" / "MotorModel_generated.h")
    parser.add_argument("--check", action="store_true", help="fail if generated output is missing or stale")
    args = parser.parse_args(argv)
    try:
        expected = render(read_model(args.model)).encode("ascii")
        if args.check:
            if not args.output.is_file() or args.output.read_bytes() != expected:
                print("Generated model is stale; run tools/generate_model.py", file=sys.stderr)
                return 1
            print("Generated model is up to date")
        else:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_bytes(expected)
            print("Generated " + str(args.output))
    except (OSError, ValueError, TypeError) as error:
        print("Model generation failed: " + str(error), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
