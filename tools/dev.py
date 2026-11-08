#!/usr/bin/env python3
"""Build and validate the portable C++03 workflow; no third-party packages."""

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
CORE = ["INFO2_P1/Motor_versionC.cpp", "INFO2_P1/Order_versionC.cpp"]


def run(command, **kwargs):
    print("+ " + shlex.join(map(str, command)), flush=True)
    subprocess.run(command, cwd=ROOT, check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("build", "test"))
    parser.add_argument("--sanitize", action="store_true",
                        help="enable GCC/Clang ASan and UBSan (Linux)")
    args = parser.parse_args()
    BUILD.mkdir(exist_ok=True)
    run([sys.executable, "tools/generate_model.py", "--check"])
    flags = ["-std=c++03", "-Wall", "-Wextra", "-Werror", "-pedantic-errors",
             "-I", "INFO2_P1"]
    if args.sanitize:
        # Non-PIE avoids ASan shadow address collisions in cloud runners.
        flags += ["-O1", "-g", "-fsanitize=address,undefined",
                  "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    else:
        flags += ["-O2"]
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    suffix = "_san" if args.sanitize else ""
    binary = BUILD / ("motor" + suffix)
    run(compiler + flags + CORE + ["INFO2_P1/MotorMain_versionC.cpp", "-o", str(binary)])
    if args.action == "test":
        unit = BUILD / ("test_motor" + suffix)
        run(compiler + flags + CORE + ["tests/test_motor.cpp", "-o", str(unit)])
        test_env = os.environ.copy()
        if args.sanitize:
            test_env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
            test_env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        run([str(unit)], env=test_env)
        test_env["MOTOR_BINARY"] = str(binary)
        run([sys.executable, "-m", "unittest", "discover", "-s", "tests", "-p", "test_*.py", "-v"],
            env=test_env)


if __name__ == "__main__":
    try:
        main()
    except (subprocess.CalledProcessError, OSError) as error:
        print("Development command failed: " + str(error), file=sys.stderr)
        sys.exit(1)
