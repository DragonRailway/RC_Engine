#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

class ConfigLoader {
public:
    static bool begin() {
        if (!LittleFS.begin(true)) {
            Serial.println("LittleFS Mount Failed");
            return false;
        }
        Serial.println("LittleFS Mounted Successfully");
        return true;
    }

    // ── File Tree ──────────────────────────────────────────────
    // Recursively prints the LittleFS directory structure with
    // indentation, file sizes, and a summary of totals.

    static void printFileTree(const char* dirPath = "/", int depth = 0) {
        File root = LittleFS.open(dirPath);
        if (!root || !root.isDirectory()) {
            Serial.printf("%*s(cannot open %s)\n", depth * 2, "", dirPath);
            return;
        }

        File entry = root.openNextFile();
        while (entry) {
            // Print indentation
            for (int i = 0; i < depth; i++) Serial.print("│ ");

            if (entry.isDirectory()) {
                Serial.printf("├─ 📁 %s/\n", entry.name());
                // Recurse into subdirectory
                String subPath = String(dirPath);
                if (!subPath.endsWith("/")) subPath += "/";
                subPath += entry.name();
                printFileTree(subPath.c_str(), depth + 1);
            } else {
                Serial.printf("├─ 📄 %-35s  %7d bytes\n", entry.name(), entry.size());
            }
            entry = root.openNextFile();
        }
    }

    // Print a full filesystem summary
    static void printFilesystemInfo() {
        Serial.println("\n╔══════════════════════════════════════════════════╗");
        Serial.println("║            LittleFS File Tree                    ║");
        Serial.println("╚══════════════════════════════════════════════════╝\n");

        printFileTree("/", 0);

        Serial.println();
        size_t total = LittleFS.totalBytes();
        size_t used  = LittleFS.usedBytes();
        Serial.println("──────────────────────────────────────────────────");
        Serial.printf("  Total: %d bytes  |  Used: %d bytes  |  Free: %d bytes\n",
                      total, used, total - used);
        Serial.printf("  Usage: %.1f%%\n", (float)used / total * 100.0f);
        Serial.println("──────────────────────────────────────────────────\n");
    }

    // ── Config File Discovery ──────────────────────────────────
    // Lists files matching a prefix pattern in a given directory.
    //   e.g. listFiles("/", "hardware-")  → hardware-tracklink.json
    //   e.g. listFiles("/sounds/", "idle-") → idle-ScaniaV8.json

    static int listFiles(const char* dirPath, const char* prefix = nullptr) {
        File dir = LittleFS.open(dirPath);
        if (!dir || !dir.isDirectory()) {
            Serial.printf("Cannot open directory: %s\n", dirPath);
            return 0;
        }

        int count = 0;
        File entry = dir.openNextFile();
        while (entry) {
            if (!entry.isDirectory()) {
                bool match = true;
                if (prefix) {
                    match = strncmp(entry.name(), prefix, strlen(prefix)) == 0;
                }
                if (match) {
                    Serial.printf("  [%d] %s/%s  (%d bytes)\n",
                                  count, dirPath, entry.name(), entry.size());
                    count++;
                }
            }
            entry = dir.openNextFile();
        }
        return count;
    }

    // ── JSON Parsing ───────────────────────────────────────────
    // Opens a JSON file and prints its top-level keys and values.

    static bool parseAndPrintJson(const char* path) {
        File file = LittleFS.open(path, "r");
        if (!file) {
            Serial.printf("  ✗ Cannot open: %s\n", path);
            return false;
        }

        Serial.printf("\n── Parsing: %s (%d bytes) ──\n", path, file.size());

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.printf("  ✗ JSON error: %s\n", error.c_str());
            return false;
        }

        // Print top-level keys
        JsonObject root = doc.as<JsonObject>();
        for (JsonPair kv : root) {
            if (kv.value().is<JsonObject>()) {
                Serial.printf("  ├─ \"%s\": { ... }\n", kv.key().c_str());
            } else if (kv.value().is<JsonArray>()) {
                Serial.printf("  ├─ \"%s\": [ %d items ]\n",
                              kv.key().c_str(), kv.value().as<JsonArray>().size());
            } else {
                // Scalar value
                String val;
                serializeJson(kv.value(), val);
                Serial.printf("  ├─ \"%s\": %s\n", kv.key().c_str(), val.c_str());
            }
        }
        Serial.printf("  ✓ OK\n");
        return true;
    }
};
