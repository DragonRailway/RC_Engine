#include "ConfigLoader.h"
#include "boards/TRACKLINK_V3.h"
#include <Arduino.h>

void printConfigInfo() {
  Serial.println("\n=== RC Brain - Config Loader Test ===\n");

  // ── 1. Mount LittleFS ──────────────────────────────────────
  if (!ConfigLoader::begin()) {
    Serial.println("FATAL: LittleFS mount failed. Halting.");
    while (1)
      delay(100);
  }

  // ── 2. Print full file tree ────────────────────────────────
  ConfigLoader::printFilesystemInfo();

  // ── 3. Discover config files by prefix ─────────────────────
  Serial.println("── Hardware Configs ──");
  int hw = ConfigLoader::listFiles("/", "hardware-");
  Serial.printf("  Found %d hardware config(s)\n\n", hw);

  Serial.println("── Vehicle Configs ──");
  int vc = ConfigLoader::listFiles("/", "vehicle-");
  Serial.printf("  Found %d vehicle config(s)\n\n", vc);

  Serial.println("── Sound Files ──");
  int snd = ConfigLoader::listFiles("/sounds/");
  Serial.printf("  Found %d sound file(s)\n\n", snd);

  Serial.println("── Sound Files: idle-* ──");
  ConfigLoader::listFiles("/sounds/", "idle-");

  Serial.println("\n── Sound Files: airbrake-* ──");
  ConfigLoader::listFiles("/sounds/", "airbrake-");

  // ── 4. Parse and validate each config JSON ─────────────────
  Serial.println("\n── Parsing Config Files ──");

  // Try to parse any hardware config found at root
  File root = LittleFS.open("/");
  if (root && root.isDirectory()) {
    File entry = root.openNextFile();
    while (entry) {
      if (!entry.isDirectory()) {
        const char *name = entry.name();
        // Parse all JSON files at root level
        size_t len = strlen(name);
        if (len > 5 && strcmp(name + len - 5, ".json") == 0) {
          String path = String("/") + name;
          ConfigLoader::parseAndPrintJson(path.c_str());
        }
      }
      entry = root.openNextFile();
    }
  }

  Serial.println("\n=== Config Loader Test Complete ===");
}

void setup() {
  Serial.begin(2000000);
  delay(10000);
}

void loop() {
  printConfigInfo();
  delay(2000);
}
