#ifndef DUMMY_LITTLEFS_H
#define DUMMY_LITTLEFS_H

#include <cstdint>
#include <cstdio>

class File {
public:
    operator bool() const { return false; }
    uint32_t getLastWrite() { return 0; }
    void close() {}
};

class DummyLittleFS {
public:
    File open(const char*, const char*) { return File(); }
};

extern DummyLittleFS LittleFS;

#endif
