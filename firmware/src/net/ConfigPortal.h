#pragma once
#include <Arduino.h>
#include <WebServer.h>

namespace mclite {
// On-demand local configuration service. All handlers run on the main loop.
class ConfigPortal {
public:
    static ConfigPortal& instance();
    bool start();
    void stop();
    void update();
    bool active() const { return _active; }
    bool restarting() const { return _restartAt != 0; }
    const String& ssid() const { return _ssid; }
    const String& password() const { return _password; }
private:
    WebServer _server{80};
    bool _active = false;
    bool _routesReady = false;
    bool _uploadOk = false;
    String _ssid, _password, _token, _upload, _error;
    uint32_t _startedAt = 0, _restartAt = 0;
    void page();
    void upload();
    void finish();
};
}
