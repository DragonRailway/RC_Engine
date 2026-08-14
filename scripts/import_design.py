#!/usr/bin/env python3
"""Import RC_UI design into RadioKit App via Remote REST API.
"""
import urllib.request
import json
import os
import sys

API_BASE = "http://127.0.0.1:17007/api"
DESIGN_FILE = "docs/radiokit-rc-ui-design.json"

def main():
    if not os.path.exists(DESIGN_FILE):
        print(f"Error: {DESIGN_FILE} not found!")
        sys.exit(1)

    with open(DESIGN_FILE, "r") as f:
        design_json_str = f.read()

    design_obj = json.loads(design_json_str)

    print("[Import Design] Uploading RC_UI design to RadioKit Remote API...")
    payload = {
        "id": "1785927365527",
        "name": "RC_UI",
        "jsonContent": design_json_str
    }

    url = f"{API_BASE}/designs"
    data = json.dumps(payload).encode('utf-8')
    req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'}, method='POST')

    try:
        with urllib.request.urlopen(req) as resp:
            res = json.loads(resp.read().decode('utf-8'))
            print(f"[Import Design] Response: {res}")
    except Exception as e:
        print(f"[Import Design] POST /api/designs error: {e}")

    # Inspect current indicator button properties in design
    print("[Import Design] Checking indicator button icons in design file...")
    pages = design_obj.get("pages", [])
    for p in pages:
        widgets = p.get("widgets", [])
        for w in widgets:
            w_name = w.get("name")
            if w_name in ["left_indicator", "right_indicator"]:
                props = w.get("properties", {})
                on_icon = props.get("onIcon")
                off_icon = props.get("offIcon")
                print(f"  Widget '{w_name}': onIcon='{on_icon}', offIcon='{off_icon}'")

if __name__ == "__main__":
    main()
