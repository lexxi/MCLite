#include "WiFiManager.h"
#include "util/log.h"

#include <WiFi.h>
#include <algorithm>

namespace mclite {

WiFiManager& WiFiManager::instance() {
    static WiFiManager inst;
    return inst;
}

int WiFiManager::scan(ScannedNetwork* out, int maxOut) {
    _lastScanResult = 0; _scanDriverStatus.store(-1); _scanDriverCount.store(-1);
    if (!out || maxOut <= 0) return 0;
    if (!_scanEventRegistered) {
        WiFi.onEvent([this](arduino_event_id_t, arduino_event_info_t info) {
            if (!_collectScanEvent.load()) return;
            _scanDriverCount.store(info.wifi_scan_done.number);
            _scanDriverStatus.store(info.wifi_scan_done.status);
        },ARDUINO_EVENT_WIFI_SCAN_DONE);
        _scanEventRegistered = true;
    }
    // Scanning is an explicit network-selection action, not background reconnect.
    // Start from a stopped driver and do not erase saved credentials during scan.
    _wantOn = false;
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    _collectScanEvent.store(false);
    WiFi.scanComplete(); // lets Arduino clear a timed-out previous scan flag
    WiFi.mode(WIFI_OFF);
    delay(150);
    WiFi.scanDelete();
    if (!WiFi.mode(WIFI_STA)) {
        _lastScanResult = -3;
        LOGLN("[WiFi] Scan: STA startup failed");
        return 0;
    }
    WiFi.setSleep(false);
    delay(250);
    _collectScanEvent.store(true);
    const int n = WiFi.scanNetworks(false,true,false,300,0); // all allowed 2.4-GHz channels, include hidden
    _lastScanResult = n;
    // Arduino releases its blocking wait before user event callbacks necessarily
    // finish. Give the diagnostic callback a short bounded opportunity to publish.
    for (unsigned i = 0; i < 20 && _scanDriverStatus.load() < 0 && n >= 0; ++i) delay(5);
    _collectScanEvent.store(false);
    LOGF("[WiFi] Scan result=%d driver=%d AP=%d mode=%d\n",n,lastScanDriverStatus(),lastScanAccessPoints(),int(WiFi.getMode()));
    if (n <= 0) { WiFi.scanDelete(); return 0; }

    int count = 0;
    for (int i = 0; i < n; ++i) {
        const String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue; // no selectable SSID for a hidden AP
        const int32_t rssi = WiFi.RSSI(i);
        int slot = -1;
        for (int j = 0; j < count; ++j) if (out[j].ssid == ssid) { slot = j; break; }
        if (slot >= 0) {
            if (rssi <= out[slot].rssi) continue;
        } else if (count < maxOut) slot = count++;
        else {
            slot = 0;
            for (int j = 1; j < count; ++j) if (out[j].rssi < out[slot].rssi) slot = j;
            if (rssi <= out[slot].rssi) continue;
        }
        out[slot].ssid = ssid;
        out[slot].rssi = rssi;
        out[slot].open = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
    std::sort(out,out + count,[](const ScannedNetwork& a,const ScannedNetwork& b) { return a.rssi > b.rssi; });
    return count;
}

bool WiFiManager::connect(const String& ssid, const String& password, uint32_t timeoutMs) {
    if (ssid.length() == 0) return false;

    // Start from a clean radio state — a prior scan / failed attempt can leave
    // STA half-open so WiFi.begin() never associates.
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.disconnect(true);
    delay(200);

    LOGF("[WiFi] connecting to '%s' (password %d chars)\n",
                  ssid.c_str(), (int)password.length());
    WiFi.begin(ssid.c_str(), password.length() ? password.c_str() : (const char*)nullptr);

    uint32_t start = millis();
    int8_t lastStatus = -1;
    while (millis() - start < timeoutMs) {
        wl_status_t st = WiFi.status();
        if (st != lastStatus) {                       // log only on change
            LOGF("[WiFi]  status=%d (%lus)\n", (int)st, (millis() - start) / 1000);
            lastStatus = st;
        }
        if (st == WL_CONNECTED) {
            _lastStatus = (int)st;
            LOGF("[WiFi] connected: ip=%s rssi=%d\n",
                          WiFi.localIP().toString().c_str(), (int)WiFi.RSSI());
            return true;
        }
        delay(150);
        yield();
    }
    _lastStatus = (int)WiFi.status();
    LOGF("[WiFi] connect timed out, final status=%d "
                  "(1=no-SSID, 4=connect-failed/bad-password, 6=disconnected)\n",
                  _lastStatus);
    return false;
}

void WiFiManager::setPersistent(bool on) {
    _wantOn = on;
    WiFi.setAutoReconnect(on);   // stack reconnects in the background after a drop
}

void WiFiManager::disconnect() {
    _wantOn = false;
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(true);   // disconnect + turn off the radio
    WiFi.mode(WIFI_OFF);
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::connectedSsid() {
    return WiFi.isConnected() ? WiFi.SSID() : String();
}

String WiFiManager::localIp() {
    return WiFi.isConnected() ? WiFi.localIP().toString() : String();
}

}  // namespace mclite
