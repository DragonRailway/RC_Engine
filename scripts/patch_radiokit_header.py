#!/usr/bin/env python3
"""
Patch the RadioKit-generated src/RADIOKIT.h to the vendored RadioKit v2.0 API.

The app's generator (GET /api/designs/<id>/header) emits a newer API than the
vendored lib/rk-arduino (v2.0.0) provides. This script rewrites the generated
header to match the vendored API and re-applies the local patches:

  1. Header guard -> RC_BRAIN_RADIOKIT_H (avoids colliding with RadioKitLib.h's
     own RADIOKIT_H guard, which would silently skip the library include).
  2. Multi-select item blocks (`rk.items[N] = {"L", N};` + `rk.itemCount = N;`)
     -> field-by-field label/pos assignment, expanded to the itemCount declared
     in docs/radiokit-rc-ui-design.json (the export omits item labels past C).
  3. Removes `X.setMode(RK_BUTTON_*);` lines (class type selects mode in v2.0).

Run after every `curl /api/designs/<id>/header > src/RADIOKIT.h`.

Usage:
    python3 scripts/patch_radiokit_header.py
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HDR = ROOT / "src" / "RADIOKIT.h"
DESIGN = ROOT / "docs" / "radiokit-rc-ui-design.json"


def main():
    if not HDR.exists():
        sys.exit(f"Missing {HDR}")
    if not DESIGN.exists():
        sys.exit(f"Missing {DESIGN}")
    text = HDR.read_text()
    original = text

    # 1. Header guard fix (must not collide with RadioKitLib.h's RADIOKIT_H)
    text = text.replace(
        "#ifndef RADIOKIT_H\n#define RADIOKIT_H",
        "#ifndef RC_BRAIN_RADIOKIT_H\n"
        "#define RC_BRAIN_RADIOKIT_H   // must not collide with RadioKitLib.h's own RADIOKIT_H guard",
    )
    text = re.sub(r"#endif // RADIOKIT_H", "#endif // RC_BRAIN_RADIOKIT_H", text)

    # 2. Multi-select item blocks -> field-by-field, expanded to declared itemCount.
    #    Item labels come from the design's onLabel when exported (e.g. gear_switch
    #    D/P/R); items beyond the exported set fall back to A/B/C letters.
    design = json.loads(DESIGN.read_text())
    declared = {}   # widget name -> declared itemCount
    labels = {}     # widget name -> list of onLabels (as exported, may be short)
    for page in design.get("pages", []):
        for w in page.get("widgets", []):
            if w.get("type") == "multiple":
                declared[w["name"]] = int(w["properties"].get("itemCount", 0))
                items = w.get("properties", {}).get("items", []) or []
                labels[w["name"]] = [it.get("onLabel") or chr(ord("A") + i)
                                      for i, it in enumerate(items)]

    for widget, count in declared.items():
        pat = re.compile(
            rf"(?P<indent>[ \t]*){re.escape(widget)}\.rk\.items\[\d+\] = \{{[^}}]*\}};"
            rf"(?:\n(?P=indent){re.escape(widget)}\.rk\.items\[\d+\] = \{{[^}}]*\}};)*"
            rf"\n(?P=indent){re.escape(widget)}\.rk\.itemCount = \d+;[^\n]*"
        )

        def rebuild(m):
            indent = m.group("indent")
            lines = []
            for i in range(count):
                if i < len(labels.get(widget, [])):
                    label = labels[widget][i]
                else:
                    label = chr(ord("A") + i)
                lines.append(f'{indent}{widget}.rk.items[{i}].label = "{label}";')
                lines.append(f"{indent}{widget}.rk.items[{i}].pos = {i};")
            lines.append(f"{indent}{widget}.rk.itemCount = {count};")
            return "\n".join(lines)

        text, n = pat.subn(rebuild, text)
        if n == 0:
            print(f"  ! no item block found for '{widget}' (skipped)")

    # 3. Remove setMode(...) lines (v2.0 has no button mode switching)
    text = re.sub(r"[ \t]*\w+\.setMode\(RK_BUTTON_\w+\);\n", "", text)

    # 4. Guard RK_ENABLE_* defines so they don't collide with platformio.ini
    #    build flags (e.g. -D RK_ENABLE_FS), which triggers a redefinition
    #    warning on every build. The replacement must end with a newline so
    #    #endif doesn't glue onto the next line.
    def guard_defines(m):
        macro = m.group(1)
        return f"#ifndef {macro}\n#define {macro}\n#endif\n"

    # Normalize first: collapse any previously-guarded block back to a bare
    # define. This keeps the script idempotent (no double-wrapping on rerun).
    text = re.sub(
        r"^#ifndef (RK_ENABLE_\w+)\n#define \1\n#endif\n",
        r"#define \1\n",
        text,
        flags=re.M,
    )
    # Then wrap any remaining bare defines (guarded ones won't match).
    text = re.sub(r"^#define (RK_ENABLE_\w+)\n", guard_defines, text, flags=re.M)

    if text == original:
        print("No changes needed — header already patched.")
    else:
        HDR.write_text(text)
        print(f"Patched {HDR}")

    # Verify no generator-only API remains
    leftovers = re.findall(r"\w+\.setMode\(|\w+\.rk\.items\[\d+\] = \{", text)
    if leftovers:
        print(f"  WARNING: leftover generator-only constructs: {leftovers}")
        sys.exit(1)


if __name__ == "__main__":
    main()
