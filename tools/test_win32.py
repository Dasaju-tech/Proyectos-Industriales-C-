#!/usr/bin/env python3
"""Cross-compile real Win32 code with MinGW and test with Wine (not MSVC 2008)."""

import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CORE = ["INFO2_P1/Motor_versionC.cpp", "INFO2_P1/Order_versionC.cpp"]


def run(command, **kwargs):
    print("+ " + shlex.join(map(str, command)), flush=True)
    subprocess.run(command, cwd=ROOT, check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--arch", choices=("win32", "win64"), default="win32")
    parser.add_argument("--compiler", help="override the selected MinGW compiler")
    parser.add_argument("--wine", default="wine")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--compile-only", action="store_true",
                        help="check Windows compilation without requiring Wine")
    args = parser.parse_args()
    if not args.compile_only and os.environ.get("WINEARCH", args.arch) != args.arch:
        parser.error("WINEARCH conflicts with --arch; unset it or select a matching architecture.")
    selected_compiler = args.compiler or (
        "i686-w64-mingw32-g++" if args.arch == "win32" else "x86_64-w64-mingw32-g++")
    compiler = shutil.which(selected_compiler)
    wine = shutil.which(args.wine)
    if not compiler or (not args.compile_only and not wine):
        parser.error("Missing MinGW or Wine. See docs/kali-vs2008.md for packages.")
    build = (args.build_dir or ROOT / "build" / args.arch).resolve()
    build.mkdir(parents=True, exist_ok=True)
    run([sys.executable, "tools/generate_model.py", "--check"])
    run([compiler, "--version"])
    if not args.compile_only:
        run([wine, "--version"])
    flags = ["-std=c++03", "-Wall", "-Wextra", "-Werror", "-pedantic-errors", "-O2",
             "-I", "INFO2_P1", "-static", "-static-libgcc", "-static-libstdc++"]
    targets = [("motor.exe", "INFO2_P1/MotorMain_versionC.cpp"),
               ("test_motor.exe", "tests/test_motor.cpp"),
               ("test_win32_timer.exe", "tests/test_win32_timer.cpp")]
    for name, source in targets:
        run([compiler] + flags + CORE + [source, "-o", str(build / name)])
    if args.compile_only:
        print(args.arch + " compilation passed; executables were NOT run.")
        return
    env = os.environ.copy()
    # Keep a dedicated, reproducible prefix; never modify the user's default.
    env.setdefault("WINEPREFIX", str(ROOT / "build" / ("wine32" if args.arch == "win32" else "wine64")))
    env["WINEPREFIX"] = str(Path(env["WINEPREFIX"]).expanduser().resolve())
    env.setdefault("WINEARCH", args.arch)
    env.setdefault("WINEDEBUG", "-all")
    print("Wine prefix: " + env["WINEPREFIX"], flush=True)
    # Initialize before tests, whose individual CLI requests have short timeouts.
    run([wine, "cmd", "/c", "exit", "0"], env=env, timeout=120)
    run([wine, str(build / "test_motor.exe")], env=env, timeout=60)
    run([wine, str(build / "test_win32_timer.exe")], env=env, timeout=60)
    launcher = build / "run_motor_wine"
    launcher.write_text("#!/bin/sh\nexec " + shlex.quote(wine) + " " +
                        shlex.quote(str(build / "motor.exe")) + ' "$@"\n', encoding="utf-8")
    launcher.chmod(0o755)
    env["MOTOR_BINARY"] = str(launcher)
    run([sys.executable, "-m", "unittest", "discover", "-s", "tests", "-p", "test_cli.py", "-v"],
        env=env, timeout=300)
    # Also exercise the interactive entry point, parsing and synchronous shutdown.
    run([wine, str(build / "motor.exe")], env=env, input="3 1\n2 10\n0\n",
        text=True, timeout=60)
    print("Win32/Wine validation passed. Native MSVC 2008 validation remains separate.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
        print("Win32 validation failed: " + str(error), file=sys.stderr)
        sys.exit(1)
