#!/usr/bin/env python3
"""Pre-build script to fetch rk-arduino library from RadioKit GitHub repo."""

import os
import shutil
import subprocess
import sys
import tempfile

try:
    from SCons.Script import Import

    Import("env")
    PROJECT_DIR = env.subst("$PROJECT_DIR")
except Exception:
    PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

LIB_RK_DIR = os.path.join(PROJECT_DIR, "lib", "rk-arduino")
HEADER_CHECK = os.path.join(LIB_RK_DIR, "src", "RadioKitLib.h")
REPO_URL = "https://github.com/Radio-Kit/rk-arduino.git"
BRANCH = "main"


def fetch_radiokit(force=False):
    if not force and os.path.exists(HEADER_CHECK):
        return

    print(
        f"\n[RadioKit Fetch] Fetching rk-arduino from {REPO_URL} (branch: {BRANCH})..."
    )
    os.makedirs(os.path.join(PROJECT_DIR, "lib"), exist_ok=True)
    temp_dir = tempfile.mkdtemp(prefix="radiokit_clone_")

    try:
        cmd_clone = [
            "git",
            "clone",
            "--depth",
            "1",
            "-b",
            BRANCH,
            REPO_URL,
            temp_dir,
        ]
        subprocess.run(
            cmd_clone, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )

        if os.path.exists(LIB_RK_DIR):
            shutil.rmtree(LIB_RK_DIR)

        shutil.copytree(temp_dir, LIB_RK_DIR, ignore=shutil.ignore_patterns(".git"))
        print(f"[RadioKit Fetch] Successfully staged rk-arduino into {LIB_RK_DIR}\n")

    except Exception as e:
        print(
            f"[RadioKit Fetch] ERROR: Failed to fetch rk-arduino: {e}", file=sys.stderr
        )
        if os.path.exists(LIB_RK_DIR):
            shutil.rmtree(LIB_RK_DIR, ignore_errors=True)
        sys.exit(1)
    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)


if __name__ == "__main__":
    force_fetch = "--force" in sys.argv or "-f" in sys.argv or "--update" in sys.argv
    fetch_radiokit(force=force_fetch)
else:
    fetch_radiokit()
