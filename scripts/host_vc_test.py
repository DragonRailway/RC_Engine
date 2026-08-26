#!/usr/bin/env python3
"""Host Vehicle Controller & Physics Harness Runner.

Compiles and executes test/host_vc/host_vc_driver.cpp natively on x86 to verify:
- VehicleController motor PWM outputs, Park Lock safety, and brake blending.
- Virtual flywheel inertia RPM acceleration/deceleration curves.
- Auxiliary hydraulic servo channels and load governor.
- Light automation and turn-signal cancellation state machines.
"""
import os
import sys
import subprocess

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
VC_SRC = os.path.join(REPO_ROOT, "test", "host_vc", "host_vc_driver.cpp")
VC_BIN = os.path.join(REPO_ROOT, "test", "host_vc", "host_vc_harness")

def main():
    print("[Host VC Test] Compiling VehicleController host harness on x86...")
    cmd = [
        "g++", "-std=c++17", "-O2",
        "-DTRACKLINK_V3",
        f"-I{os.path.join(REPO_ROOT, 'test', 'host_vc')}",
        f"-I{os.path.join(REPO_ROOT, 'src')}",
        f"-I{os.path.join(REPO_ROOT, 'boards')}",
        f"-I{os.path.join(REPO_ROOT, 'common')}",
        f"-I{os.path.join(REPO_ROOT, '..', 'ESP32_EasyKit', 'src')}",
        f"-I{os.path.join(REPO_ROOT, '..', 'RadioKit', 'rk-arduino', 'src')}",
        f"-I{os.path.join(REPO_ROOT, 'lib', 'SoundEngine', 'src')}",
        VC_SRC,
        os.path.join(REPO_ROOT, "test", "host_vc", "host_easykit_stubs.cpp"),
        os.path.join(REPO_ROOT, "lib", "SoundEngine", "src", "RcEngineSound.cpp"),
        os.path.join(REPO_ROOT, "..", "RadioKit", "rk-arduino", "src", "widgets", "Widget.cpp"),
        os.path.join(REPO_ROOT, "..", "RadioKit", "rk-arduino", "src", "widgets", "Button.cpp"),
        os.path.join(REPO_ROOT, "..", "RadioKit", "rk-arduino", "src", "widgets", "LED.cpp"),
        os.path.join(REPO_ROOT, "..", "RadioKit", "rk-arduino", "src", "widgets", "Multiple.cpp"),
        os.path.join(REPO_ROOT, "..", "RadioKit", "rk-arduino", "src", "widgets", "Telemetry.cpp"),
        os.path.join(REPO_ROOT, "..", "RadioKit", "rk-arduino", "src", "widgets", "Text.cpp"),
        os.path.join(REPO_ROOT, "..", "RadioKit", "rk-arduino", "src", "widgets", "Knob.cpp"),
        os.path.join(REPO_ROOT, "..", "RadioKit", "rk-arduino", "src", "widgets", "Slider.cpp"),
        os.path.join(REPO_ROOT, "..", "RadioKit", "rk-arduino", "src", "widgets", "SlideSwitch.cpp"),
        os.path.join(REPO_ROOT, "..", "RadioKit", "rk-arduino", "src", "widgets", "Joystick.cpp"),
        "-o", VC_BIN
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("[Host VC Test] Compilation FAIL:")
        print(res.stderr)
        sys.exit(1)

    print("[Host VC Test] Compilation SUCCESS. Executing physics harness...")
    res_run = subprocess.run([VC_BIN], capture_output=True, text=True)
    print(res_run.stdout)

    if res_run.returncode == 0:
        print("[Host VC Test] ALL Layer 1 Physics & VC assertions PASSED.")
        sys.exit(0)
    else:
        print("[Host VC Test] Harness Execution FAIL.")
        print(res_run.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
