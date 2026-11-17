#pragma once
#include <Arduino.h>
#include <functional>
#include <vector>
#include <string>
enum { WIFI_OFF=0, WIFI_STA=1, WIFI_AUTH_OPEN=0, WIFI_AUTH_WPA2_PSK=3, ARDUINO_EVENT_WIFI_SCAN_DONE=1 };
typedef int arduino_event_id_t;
struct arduino_event_info_t { struct { unsigned status; unsigned number; } wifi_scan_done; };
typedef int wl_status_t;
static constexpr int WL_CONNECTED=3;
struct WiFiScanRow { String ssid; int32_t rssi; int auth; };
class NativeWiFi {
public:
    std::vector<WiFiScanRow> rows;
    std::vector<std::string> calls;
    std::function<void(arduino_event_id_t,arduino_event_info_t)> callback;
    int result=0, driverStatus=0, driverCount=0, currentMode=0;
    bool modeSucceeds=true, hidden=false;
    void persistent(bool) { calls.push_back("persistent"); }
    void setAutoReconnect(bool on) { calls.push_back(on ? "reconnect-on":"reconnect-off"); }
    bool mode(int value) { calls.push_back(value ? "sta":"off"); currentMode=value; return value==0 || modeSucceeds; }
    int getMode() { return currentMode; }
    void setSleep(bool) { calls.push_back("sleep"); }
    int scanComplete() { return -2; }
    void scanDelete() { calls.push_back("delete"); }
    int onEvent(std::function<void(arduino_event_id_t,arduino_event_info_t)> cb,int) { callback=cb; return 1; }
    int scanNetworks(bool async,bool showHidden,bool passive,unsigned duration,unsigned channel) {
        calls.push_back("scan"); hidden=showHidden;
        if(async || passive || duration!=300 || channel!=0) return -99;
        if(callback && result>=0) { arduino_event_info_t info={}; info.wifi_scan_done.status=driverStatus; info.wifi_scan_done.number=driverCount; callback(1,info); }
        return result;
    }
    String SSID(int i) { return rows.at(i).ssid; }
    String SSID() { return "connected"; }
    int32_t RSSI(int i) { return rows.at(i).rssi; }
    int32_t RSSI() { return -60; }
    int encryptionType(int i) { return rows.at(i).auth; }
    bool disconnect(bool=false,bool=false) { calls.push_back("disconnect"); return true; }
    void begin(const char*,const char*) {}
    int status() { return WL_CONNECTED; }
    bool isConnected() { return true; }
    struct Ip { String toString() { return "192.168.1.2"; } };
    Ip localIP() { return {}; }
};
static NativeWiFi WiFi;
inline void delay(unsigned) {}
inline void yield() {}
