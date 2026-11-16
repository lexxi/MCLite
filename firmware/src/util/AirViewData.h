#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>

namespace mclite {
// Session-only bounded buffers; all access is on the main loop, never the ISR.
class AirViewData {
public:
    static constexpr size_t SAMPLE_CAP = 96;
    static constexpr size_t EVENT_CAP = 24;
    static constexpr uint32_t SAMPLE_MS = 100;
    static constexpr uint32_t WINDOW_MS = SAMPLE_CAP * SAMPLE_MS;
    struct Sample { uint32_t time = 0; float rssi = 0; bool valid = false; };
    struct Event {
        uint32_t time = 0;
        float rssi = 0, snr = 0;
        uint16_t length = 0;
        uint8_t type = 0;
        bool tx = false, failed = false;
    };
    struct RadioSettings {
        float frequency = 0, bandwidth = 0;
        uint8_t sf = 0, cr = 0;
        int8_t power = 0;
    } radio;
    static AirViewData& instance() { static AirViewData data; return data; }
    void begin() {
        _sampleNext = _sampleCount = _eventNext = _eventCount = 0;
        rxCount = txCount = failCount = 0;
        for (auto& count : types) count = 0;
        _scheduled = false; _active = true;
    }
    void setActive(bool active) {
        if (_active != active) _scheduled = false;
        _active = active;
    }
    bool active() const { return _active; }
    bool sampleDue(uint32_t now) {
        if (!_active || (_scheduled && uint32_t(now - _lastSample) < SAMPLE_MS)) return false;
        _lastSample = now; _scheduled = true;
        return true;
    }
    static bool validRssi(float rssi) { return std::isfinite(rssi) && rssi >= -150 && rssi < 0; }
    void sample(uint32_t now, bool receiving, float rssi) {
        if (!_active) return;
        auto& sample = _samples[_sampleNext];
        sample.time = now; sample.rssi = rssi; sample.valid = receiving && validRssi(rssi);
        _sampleNext = (_sampleNext + 1) % SAMPLE_CAP;
        if (_sampleCount < SAMPLE_CAP) ++_sampleCount;
    }
    void packet(uint32_t now, uint8_t type, uint16_t length, float rssi, float snr,
                bool tx = false, bool failed = false) {
        if (!_active) return;
        type &= 15;
        auto& event = _events[_eventNext];
        event.time = now; event.rssi = rssi; event.snr = snr;
        event.length = length; event.type = type; event.tx = tx; event.failed = failed;
        _eventNext = (_eventNext + 1) % EVENT_CAP;
        if (_eventCount < EVENT_CAP) ++_eventCount;
        if (failed) ++failCount;
        else if (tx) ++txCount;
        else { ++rxCount; ++types[type]; }
    }
    size_t sampleCount() const { return _sampleCount; }
    size_t eventCount() const { return _eventCount; }
    // Oldest to newest; callers bound index by the corresponding count.
    const Sample& sampleAt(size_t index) const { return _samples[(_sampleNext + SAMPLE_CAP - _sampleCount + index) % SAMPLE_CAP]; }
    const Event& eventAt(size_t index) const { return _events[(_eventNext + EVENT_CAP - _eventCount + index) % EVENT_CAP]; }
    static const char* typeName(uint8_t type) {
        switch (type) {
            case 0: return "REQ"; case 1: return "RESP"; case 2: return "DM";
            case 3: return "ACK"; case 4: return "ADV"; case 5: return "CH";
            case 6: return "DATA"; case 7: return "ANON"; case 8: return "PATH";
            case 9: return "TRACE"; case 10: return "MULTI"; case 11: return "CTRL";
            case 15: return "RAW"; default: return "?";
        }
    }
    uint32_t rxCount = 0, txCount = 0, failCount = 0;
    uint32_t types[16] = {};
private:
    bool _active = false, _scheduled = false;
    uint32_t _lastSample = 0;
    Sample _samples[SAMPLE_CAP] = {};
    Event _events[EVENT_CAP] = {};
    size_t _sampleNext = 0, _sampleCount = 0, _eventNext = 0, _eventCount = 0;
};
}
