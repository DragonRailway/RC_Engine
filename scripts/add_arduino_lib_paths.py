Import("env")
import os

try:
    platform = env.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif32")
    if framework_dir:
        for lib in ["FS", "LittleFS", "Update"]:
            lib_path = os.path.join(framework_dir, "libraries", lib, "src")
            if os.path.exists(lib_path):
                env.Append(CPPPATH=[lib_path])
except Exception as e:
    print(f"[add_arduino_lib_paths] Notice: {e}")
