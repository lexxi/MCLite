#include "ConfigPortal.h"
#include "WiFiManager.h"
#include "../config/ConfigManager.h"
#include "../config/defaults.h"
#include "../storage/SDCard.h"
#include "../companion/CompanionService.h"
#include <WiFi.h>
#include <esp_system.h>

namespace mclite {
namespace {
constexpr size_t MAX_UPLOAD = 32768; // ConfigManager's boot-time read limit
constexpr uint32_t TIMEOUT_MS = 10 * 60 * 1000;
String randomHex() {
    char b[17];
    snprintf(b, sizeof(b), "%08lx%08lx", (unsigned long)esp_random(), (unsigned long)esp_random());
    return String(b);
}
}
ConfigPortal& ConfigPortal::instance() { static ConfigPortal p; return p; }
bool ConfigPortal::start() {
    if (_active) return true;
    auto& cfg = ConfigManager::instance().config();
    auto& comp = CompanionService::instance();
    if (!cfg.security.adminEnabled || cfg.permissions.settings != "full" ||
        comp.bleInited() || !SDCard::instance().isMounted()) return false;
    comp.setBleCompanionEnabled(false);
    comp.setWifiCompanionEnabled(false);
    comp.setUsbCompanionEnabled(false);
    WiFiManager::instance().disconnect();
    _ssid = "MCLite-Config-" + randomHex().substring(0, 6);
    _password = randomHex();
    _token = randomHex() + randomHex();
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP);
    IPAddress ip(192, 168, 4, 1);
    if (!WiFi.softAPConfig(ip, ip, IPAddress(255,255,255,0)) ||
        !WiFi.softAP(_ssid.c_str(), _password.c_str(), 1, false, 1)) {
        WiFi.mode(WIFI_OFF);
        return false;
    }
    if (!_routesReady) {
    const char* headers[] = {"X-Config-Token"};
    _server.collectHeaders(headers, 1);
    _server.on("/", HTTP_GET, [this] { page(); });
    _server.on("/upload", HTTP_POST, [this] { finish(); }, [this] { upload(); });
    _server.onNotFound([this] { _server.send(404, "text/plain", "Not found"); });
    _routesReady = true;
    }
    _server.begin();
    _startedAt = millis();
    _restartAt = 0;
    _active = true;
    return true;
}
void ConfigPortal::stop() {
    if (!_active || restarting()) return;
    _server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    _active = false;
    _upload = "";
    _password = "";
    _token = "";
}
void ConfigPortal::update() {
    if (!_active) return;
    if (_restartAt) {
        if ((int32_t)(millis() - _restartAt) >= 0) ESP.restart();
        return;
    }
    if (millis() - _startedAt >= TIMEOUT_MS) { stop(); return; }
    _server.handleClient();
}
void ConfigPortal::page() {
    _server.sendHeader("Cache-Control", "no-store");
    _server.sendHeader("X-Frame-Options", "DENY");
    String html = R"HTML(<!doctype html><html lang="de"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MCLite Konfiguration</title><style>body{font:18px system-ui;max-width:38em;margin:2em auto;padding:1em;background:#14202c;color:#fff}button,input{font:inherit;margin:1em 0;max-width:100%}button{padding:.6em}p{line-height:1.5}</style>
<h1>MCLite Konfiguration</h1><p>config.json auswählen. Die bisherige Konfiguration wird vor dem Ersetzen gesichert. Anschließend startet das Gerät neu.</p>
<p>Ohne identity-Abschnitt bleibt die Identität dieses Geräts erhalten. Eine mitgelieferte Identität ersetzt sie.</p>
<input id="file" type="file" accept=".json,application/json"><br><button id="send">Prüfen und übernehmen</button><p id="status"></p>
<script>const token=')HTML";
    html += _token;
    html += R"HTML(';document.getElementById('send').onclick=async()=>{
const f=document.getElementById('file').files[0],s=document.getElementById('status'),b=document.getElementById('send');
if(!f){s.textContent='Bitte config.json auswählen.';return}if(f.size>32768){s.textContent='Maximal 32 KiB.';return}
if(!confirm('Konfiguration ersetzen und Gerät neu starten?'))return;
b.disabled=true;try{const d=new FormData();d.append('config',f);const r=await fetch('/upload',{method:'POST',headers:{'X-Config-Token':token},body:d});s.textContent=await r.text();if(!r.ok)b.disabled=false}catch(e){s.textContent='Verbindung unterbrochen. Gerätestatus prüfen.';b.disabled=false}};</script></html>)HTML";
    _server.send(200, "text/html; charset=utf-8", html);
}
void ConfigPortal::upload() {
    auto& u = _server.upload();
    if (u.status == UPLOAD_FILE_START) {
        _upload = "";
        _error = "";
        _uploadOk = false;
        if (_server.header("X-Config-Token") != _token) _error = "Zugriff abgelehnt.";
        else if (u.name != "config") _error = "Ungültiges Upload-Feld.";
    } else if (u.status == UPLOAD_FILE_WRITE && !_error.length()) {
        if (u.currentSize > MAX_UPLOAD - _upload.length()) _error = "Maximal 32 KiB.";
        else if (!_upload.concat((const char*)u.buf, u.currentSize)) _error = "Nicht genug Speicher.";
    } else if (u.status == UPLOAD_FILE_END) {
        _uploadOk = !_error.length() && _upload.length() > 0;
    } else if (u.status == UPLOAD_FILE_ABORTED) {
        _error = "Upload abgebrochen.";
        _upload = "";
        _uploadOk = false;
    }
}
void ConfigPortal::finish() {
    _server.sendHeader("Cache-Control", "no-store");
    if (_server.header("X-Config-Token") != _token) {
        _server.send(403, "text/plain; charset=utf-8", "Zugriff abgelehnt."); return;
    }
    if (!_uploadOk) {
        _server.send(400, "text/plain; charset=utf-8", _error.length() ? _error : "Keine vollständige Datei empfangen.");
        _upload = ""; return;
    }
    _uploadOk = false;
    String error;
    bool ok = ConfigManager::instance().importJson(_upload, error);
    _upload = "";
    if (!ok) { _server.send(400, "text/plain; charset=utf-8", error); return; }
    _server.send(200, "text/plain; charset=utf-8", "Gesichert und übernommen. Das T-Deck startet neu.");
    _restartAt = millis() + 1500;
}
}
