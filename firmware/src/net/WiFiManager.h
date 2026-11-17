#pragma once

#include <Arduino.h>
#include <atomic>

// On-demand WiFi (STA) helper for the firmware auto-update feature. WiFi is
// brought up only for a scan / update-check / download and torn down after, to
// avoid battery drain and CPU/RAM contention with the LoRa mesh. WiFi (on-chip
// 2.4 GHz) does not share the SPI bus with the SX1262 LoRa radio.

namespace mclite {

struct ScannedNetwork {
    String  ssid;
    int32_t rssi = 0;
    bool    open = false;  // true = no password required
};

class WiFiManager {
public:
    static WiFiManager& instance();

    // Blocking scan (~2-4 s). Fills up to maxOut entries (named, de-duplicated,
    // sorted by signal strength). Returns the number written.
    int scan(ScannedNetwork* out, int maxOut);
    int lastScanResult() const { return _lastScanResult; } // Arduino result; -3 = STA startup failed
    int lastScanDriverStatus() const { return _scanDriverStatus.load(); }
    int lastScanAccessPoints() const { return _scanDriverCount.load(); }
    bool scanFailed() const {
        return _lastScanResult < 0 || _scanDriverStatus.load() > 0 ||
               (_lastScanResult == 0 && _scanDriverCount.load() > 0);
    }

    // Blocking connect with timeout. Returns true once associated (WL_CONNECTED).
    bool connect(const String& ssid, const String& password, uint32_t timeoutMs = 15000);
    void disconnect();   // drop the link and power the radio off

    bool   isConnected();
    String connectedSsid();
    String localIp();

    // "Persistent" intent: when on, keep WiFi up and let the ESP32 stack
    // auto-reconnect in the background after a drop (non-blocking). Set true
    // after the user toggles WiFi on; cleared by disconnect().
    void   setPersistent(bool on);
    bool   wantsConnection() const { return _wantOn; }

    // Last wl_status_t from connect() (e.g. 1=no SSID, 4=auth/bad password, 6=disconnected).
    int    lastStatus() const { return _lastStatus; }

private:
    WiFiManager() = default;
    int _lastScanResult = 0;
    bool _scanEventRegistered = false;
    std::atomic<bool> _collectScanEvent{false};
    std::atomic<int> _scanDriverStatus{-1}, _scanDriverCount{-1};
    int  _lastStatus = 0;
    bool _wantOn = false;
};

}  // namespace mclite
