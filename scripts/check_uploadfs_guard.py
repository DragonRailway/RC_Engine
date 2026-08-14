#!/usr/bin/env python3
"""Guardrail: fail fast when someone runs `pio run -t uploadfs`.

The config-bundles model moved all deployable configs and sounds out of `data/`
into `configs/` (hardware_configs/ + vehicle_configs/). PlatformIO's `uploadfs`
target builds the LittleFS image from the hardcoded `data/` directory, which is
now gitignored scratch — a naked `uploadfs` would produce an empty/near-empty
filesystem and the board would boot FATAL (`Cannot open: /hardware-<BOARD>.json`).

The only supported FS-flash path is `scripts/build_fs.py --board <B> --vehicle <V>`.

Wired as `pre:scripts/check_uploadfs_guard.py` in platformio.ini; runs before
every build and aborts with a clear message when a filesystem target
(uploadfs / uploadfsota / buildfs) is requested.
"""
import sys
from pathlib import Path

from SCons.Script import COMMAND_LINE_TARGETS

Import("env")  # noqa: F821  (injected by SCons)

DATA = Path(env.subst("$PROJECT_DIR")) / "data"


def guard(target, source, **kw):  # noqa: ARG001
    e = kw.get("env") or kw.get("env_") or env
    # Data dir may not exist at all (scratch is disposable).
    if not DATA.is_dir():
        sys.stderr.write(
            "uploadfs blocked: data/ does not exist. "
            "Use `scripts/build_fs.py --board <B> --vehicle <V>` to deploy the FS.\n")
        e.Exit(1)
        return

    configs = list(DATA.glob("hardware-*.json")) + list(DATA.glob("vehicle-config.json"))
    if not configs:
        sys.stderr.write(
            "uploadfs blocked: data/ contains no configs (it is gitignored scratch). "
            "Use `scripts/build_fs.py --board <B> --vehicle <V>` to deploy the FS.\n")
        e.Exit(1)


if {"uploadfs", "uploadfsota", "buildfs"} & set(COMMAND_LINE_TARGETS):
    env.AddPreAction("uploadfs", guard)
    env.AddPreAction("uploadfsota", guard)
    env.AddPreAction("buildfs", guard)
