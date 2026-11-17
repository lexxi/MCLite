#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <utility>
#ifdef ARDUINO_ARCH_ESP32
#include <esp_heap_caps.h>
#endif

namespace mclite {
struct DirectoryNode {
    uint8_t key[32];
    char name[48];
    char lastSeen[21];
    int32_t latE6, lonE6;
    uint8_t type;
    bool hasPosition;
};
// Fixed, checked PSRAM allocation. No growing vector/allocator exceptions on ESP32.
class DirectoryTable {
public:
    static constexpr size_t CAP = 4096;
    DirectoryTable() = default;
    DirectoryTable(const DirectoryTable&) = delete;
    DirectoryTable& operator=(const DirectoryTable&) = delete;
    ~DirectoryTable() { release(); }
    bool allocate() {
        if (_nodes) return true;
#ifdef ARDUINO_ARCH_ESP32
        _nodes = static_cast<DirectoryNode*>(heap_caps_calloc(CAP, sizeof(DirectoryNode), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
        _nodes = static_cast<DirectoryNode*>(std::calloc(CAP, sizeof(DirectoryNode)));
#endif
        return _nodes != nullptr;
    }
    void release() {
#ifdef ARDUINO_ARCH_ESP32
        heap_caps_free(_nodes);
#else
        std::free(_nodes);
#endif
        _nodes = nullptr; _count = 0;
    }
    void swap(DirectoryTable& other) { std::swap(_nodes, other._nodes); std::swap(_count, other._count); }
    size_t count() const { return _count; }
    const DirectoryNode* entries() const { return _nodes; }
    const DirectoryNode* find(const uint8_t* key) const {
        if (!key) return nullptr;
        for (size_t i = 0; i < _count; ++i) if (!memcmp(_nodes[i].key, key, 32)) return &_nodes[i];
        return nullptr;
    }
    bool upsert(const DirectoryNode& node) {
        if (!_nodes) return false;
        for (size_t i = 0; i < _count; ++i) if (!memcmp(_nodes[i].key, node.key, 32)) { _nodes[i] = node; return true; }
        if (_count == CAP) return false;
        _nodes[_count++] = node;
        return true;
    }
    static int nibble(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }
    static bool decodeKey(const char* hex, uint8_t* key) {
        if (!hex || !key || strlen(hex) != 64) return false;
        bool any = false;
        for (size_t i = 0; i < 32; ++i) {
            int a = nibble(hex[i * 2]), b = nibble(hex[i * 2 + 1]);
            if (a < 0 || b < 0) return false;
            key[i] = a * 16 + b; any |= key[i] != 0;
        }
        return any;
    }
    static bool validPosition(double lat, double lon) {
        return std::isfinite(lat) && std::isfinite(lon) && lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180;
    }
    static uint8_t role(const char* name) {
        if (!name) return 0;
        if (!strcmp(name,"companion")) return 1;
        if (!strcmp(name,"repeater")) return 2;
        if (!strcmp(name,"room") || !strcmp(name,"room_server")) return 3;
        if (!strcmp(name,"sensor")) return 4;
        return 0; // unknown types still count for prefix ambiguity
    }
    // UTF-8-safe truncation and no control/newline characters in UI names.
    static void copyName(char* dest, size_t capacity, const char* source) {
        if (!capacity) return;
        if (!source) { dest[0] = 0; return; }
        size_t length = strlen(source), n = length < capacity - 1 ? length : capacity - 1;
        if (n < length) while (n && (static_cast<unsigned char>(source[n]) & 0xc0) == 0x80) --n;
        memcpy(dest, source, n); dest[n] = 0;
        for (size_t i = 0; i < n; ++i) if (static_cast<unsigned char>(dest[i]) < 32 || dest[i] == 127) dest[i] = ' ';
    }
private:
    DirectoryNode* _nodes = nullptr;
    size_t _count = 0;
};
}
