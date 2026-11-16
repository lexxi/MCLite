#pragma once
#include <Arduino.h>
#include <cstdint>
#include <cstring>
#include <vector>

namespace mclite {
// hashSize == 0 means unavailable (including history from older firmware).
// An empty path with hashSize != 0 is a captured, directly received flood.
struct MessagePath {
    uint8_t hashSize = 0;
    String hex;
    static int nibble(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }
    bool valid(uint8_t hops) const {
        if (hashSize < 1 || hashSize > 4 || hops > 63) return false;
        size_t bytes = size_t(hops) * hashSize;
        if (bytes > 64 || hex.length() != bytes * 2) return false;
        for (size_t i = 0; i < hex.length(); ++i) if (nibble(hex[i]) < 0) return false;
        return true;
    }
    static MessagePath capture(const uint8_t* bytes, uint8_t width, uint8_t hops) {
        MessagePath result;
        size_t len = size_t(width) * hops;
        if (width < 1 || width > 4 || hops > 63 || len > 64 || (len && !bytes)) return result;
        char encoded[129];
        const char* digits = "0123456789abcdef";
        for (size_t i = 0; i < len; ++i) {
            encoded[i * 2] = digits[bytes[i] >> 4];
            encoded[i * 2 + 1] = digits[bytes[i] & 15];
        }
        encoded[len * 2] = 0;
        result.hashSize = width;
        result.hex = encoded;
        return result;
    }
    // Caller first validates the entire path. Match ALL known keys, including
    // keys without GPS: otherwise a collision could appear uniquely located.
    bool matches(uint8_t hop, const uint8_t* key) const {
        if (!key || hashSize < 1 || hashSize > 4) return false;
        size_t start = size_t(hop) * hashSize * 2;
        if (start + hashSize * 2 > hex.length()) return false;
        for (size_t j = 0; j < hashSize; ++j) {
            int hi = nibble(hex[start + j * 2]), lo = nibble(hex[start + j * 2 + 1]);
            if (hi < 0 || lo < 0 || key[j] != (hi * 16 + lo)) return false;
        }
        return true;
    }
};
// Full-key deduplication must precede prefix-collision counting. GPS availability
// deliberately plays no role in choosing the candidates.
inline bool addMessagePathCandidate(const MessagePath& path, uint8_t hop,
                                    std::vector<const uint8_t*>& keys, const uint8_t* key) {
    if (!path.matches(hop, key)) return false;
    for (const auto* known : keys) if (memcmp(known, key, 32) == 0) return false;
    keys.push_back(key);
    return true;
}
} // namespace mclite
