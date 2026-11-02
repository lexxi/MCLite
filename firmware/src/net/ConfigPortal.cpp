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
    _server.on("/tiles/upload", HTTP_POST, [this] { tileFinish(); }, [this] { tileUpload(); });
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
    tileCleanup();
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
<hr><h2>Offline-Karten</h2><p>Den entpackten tiles-Ordner auswählen (Zoom/X/Y.png, 256 × 256 Pixel). Vorhandene Kacheln werden übersprungen. Unterbrochene Übertragungen können erneut gestartet werden. Kein ZIP hochladen.</p>
<input id="tiles" type="file" webkitdirectory multiple><br><button id="sendTiles">Karten übertragen</button><button id="cancelTiles" disabled>Nach dieser Kachel stoppen</button><p id="tileStatus"></p>
<script>const token=')HTML";
    html += _token;
    html += R"HTML(';document.getElementById('send').onclick=async()=>{
const f=document.getElementById('file').files[0],s=document.getElementById('status'),b=document.getElementById('send');
if(!f){s.textContent='Bitte config.json auswählen.';return}if(f.size>32768){s.textContent='Maximal 32 KiB.';return}
if(!confirm('Konfiguration ersetzen und Gerät neu starten?'))return;
b.disabled=true;document.getElementById('sendTiles').disabled=true;try{const d=new FormData();d.append('config',f);const r=await fetch('/upload',{method:'POST',headers:{'X-Config-Token':token},body:d});s.textContent=await r.text();if(!r.ok)b.disabled=false}catch(e){s.textContent='Verbindung unterbrochen. Gerätestatus prüfen.';b.disabled=false}document.getElementById('sendTiles').disabled=false};

let cancelTiles=false;
document.getElementById('cancelTiles').onclick=()=>{cancelTiles=true};
document.getElementById('sendTiles').onclick=async()=>{
const files=Array.from(document.getElementById('tiles').files),s=document.getElementById('tileStatus'),b=document.getElementById('sendTiles'),cancel=document.getElementById('cancelTiles');
const selected=[];
for(const f of files){
const m=(f.webkitRelativePath||f.name).match(/(?:^|\/)(\d+)\/(\d+)\/(\d+)\.png$/i);
if(!m)continue;
const [z,x,y]=m.slice(1).map(Number);
if(z>19||x>=2**z||y>=2**z||f.size<24||f.size>1048576){s.textContent='Ungültige Kachel: '+f.webkitRelativePath;return}
selected.push({f,name:z+'_'+x+'_'+y+'.png'});
}
if(!selected.length){s.textContent='Keine Kacheln im Format Zoom/X/Y.png gefunden.';return}
b.disabled=true;document.getElementById('send').disabled=true;cancel.disabled=false;cancelTiles=false;
let done=0,skipped=0;
try{
for(const item of selected){
if(cancelTiles)break;
s.textContent=(done+skipped)+' / '+selected.length+' – '+item.name;
const d=new FormData();d.append('tile',item.f,item.name);
const r=await fetch('/tiles/upload',{method:'POST',headers:{'X-Config-Token':token},body:d});
if(r.status===409)skipped++;else if(r.ok)done++;else throw new Error(await r.text());
}
s.textContent=(cancelTiles?'Gestoppt. ':'Fertig. ')+done+' übertragen, '+skipped+' vorhanden. '+(done+skipped)+' / '+selected.length+'. Nach Abschluss T-Deck neu starten, um Karten neu einzulesen.';
}catch(e){s.textContent='Übertragung gestoppt: '+e.message+'. Ordner erneut übertragen; vorhandene Kacheln werden übersprungen.'}
finally{b.disabled=false;document.getElementById('send').disabled=false;cancel.disabled=true}
};
</script></html>)HTML";
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

void ConfigPortal::tileCleanup() {
    if (_tileFile) _tileFile.close();
    SD.remove("/tiles-upload.tmp");
}
void ConfigPortal::tileUpload() {
    auto& u = _server.upload();
    _startedAt = millis(); // timeout measures inactivity, not total transfer time
    if (u.status == UPLOAD_FILE_START) {
        tileCleanup();
        _tileError = ""; _tilePath = ""; _tileBytes = 0;
        _tileOk = false; _tileExists = false;
        memset(_tileHeader, 0, sizeof(_tileHeader));
        if (_server.header("X-Config-Token") != _token) {
            _tileError = "Zugriff abgelehnt."; return;
        }
        if (u.name != "tile") { _tileError = "Ungültiges Upload-Feld."; return; }
        // Browser converts the selected directory path to this restricted name.
        unsigned z, x, y; int used = 0;
        if (sscanf(u.filename.c_str(), "%u_%u_%u.png%n", &z, &x, &y, &used) != 3 ||
            used != (int)u.filename.length() || z > 19 ||
            x >= (1UL << z) || y >= (1UL << z)) {
            _tileError = "Ungültige Kacheladresse."; return;
        }
        String dir = "/tiles/" + String(z);
        String column = dir + "/" + String(x);
        _tilePath = column + "/" + String(y) + ".png";
        if (SD.exists(_tilePath.c_str())) { _tileExists = true; return; }
        if ((!SD.exists("/tiles") && !SD.mkdir("/tiles")) ||
            (!SD.exists(dir.c_str()) && !SD.mkdir(dir.c_str())) ||
            (!SD.exists(column.c_str()) && !SD.mkdir(column.c_str()))) {
            _tileError = "SD-Verzeichnis konnte nicht erstellt werden."; return;
        }
        _tileFile = SD.open("/tiles-upload.tmp", FILE_WRITE);
        if (!_tileFile) _tileError = "SD-Datei konnte nicht geöffnet werden.";
    } else if (u.status == UPLOAD_FILE_WRITE && !_tileError.length() && !_tileExists) {
        if (u.currentSize > 1048576UL - _tileBytes) {
            _tileError = "Kachel größer als 1 MiB."; tileCleanup(); return;
        }
        for (size_t i = 0; i < u.currentSize && _tileBytes + i < sizeof(_tileHeader); ++i)
            _tileHeader[_tileBytes + i] = u.buf[i];
        if (!_tileFile || _tileFile.write(u.buf, u.currentSize) != u.currentSize) {
            _tileError = "SD-Schreibfehler (Karte voll?)."; tileCleanup(); return;
        }
        _tileBytes += u.currentSize;
    } else if (u.status == UPLOAD_FILE_END && !_tileError.length() && !_tileExists) {
        const uint8_t prefix[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82};
        const uint8_t dimensions[] = {0,0,1,0,0,0,1,0};
        if (_tileBytes < 24 || _tileBytes != u.totalSize ||
            memcmp(_tileHeader, prefix, sizeof(prefix)) ||
            memcmp(_tileHeader + 16, dimensions, sizeof(dimensions))) {
            _tileError = "Erwartet wird eine 256x256-PNG-Kachel."; tileCleanup(); return;
        }
        _tileFile.flush();
        _tileFile.close();
        File check = SD.open("/tiles-upload.tmp", FILE_READ);
        bool valid = check && check.size() == _tileBytes;
        check.close();
        if (!valid || !SD.rename("/tiles-upload.tmp", _tilePath.c_str())) {
            _tileError = "Kachel konnte nicht gespeichert werden."; tileCleanup(); return;
        }
        _tileOk = true;
    } else if (u.status == UPLOAD_FILE_ABORTED) {
        _tileError = "Upload abgebrochen."; _tileOk = false; tileCleanup();
    }
}
void ConfigPortal::tileFinish() {
    _server.sendHeader("Cache-Control", "no-store");
    if (_server.header("X-Config-Token") != _token) {
        _server.send(403, "text/plain; charset=utf-8", "Zugriff abgelehnt."); return;
    }
    int code = _tileError.length() ? 400 : (_tileExists ? 409 : (_tileOk ? 200 : 400));
    String response = _tileError.length() ? _tileError :
        (_tileExists ? "Kachel bereits vorhanden." : (_tileOk ? "Kachel gespeichert." : "Keine vollständige Kachel empfangen."));
    _tileOk = false;
    _server.send(code, "text/plain; charset=utf-8", response);
}

}
