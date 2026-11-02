#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <SD.h>

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
    File _tileFile;
    String _tilePath, _tileError;
    size_t _tileBytes = 0;
    uint8_t _tileHeader[24] = {};
    bool _tileOk = false, _tileExists = false;
    void tileUpload();
    void tileFinish();
    void tileCleanup();
    void page();
    void upload();
    void finish();
};
}
