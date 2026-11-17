#include "NodeDirectoryDownload.h"
#include "WiFiManager.h"
#include "../storage/NodeDirectory.h"
#include "../storage/SDCard.h"
#include "../util/DirectoryJson.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace mclite {
extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
namespace {
// HTTPClient de-chunks into this sink. Bound the body even without Content-Length.
class BoundedBody : public Stream {
public:
    String text;
    bool overflow = false;
    static constexpr size_t CAP = 96 * 1024;
    size_t write(uint8_t byte) override { return write(&byte,1); }
    size_t write(const uint8_t* data,size_t length) override {
        if (length > CAP - text.length()) { overflow = true; return 0; }
        return text.concat(reinterpret_cast<const char*>(data),length) ? length : 0;
    }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
};
}
NodeDirectoryDownload& NodeDirectoryDownload::instance() { static NodeDirectoryDownload download; return download; }
bool NodeDirectoryDownload::start() {
    if (busy()) return false;
    if (!WiFiManager::instance().isConnected()) { fail(Error::Offline); return false; }
    if (!SDCard::instance().isMounted()) { fail(Error::Storage); return false; }
    _pending.release(); _progress.store(0); _saved.store(0); _cancel.store(false); _error.store(Error::None);
    _state.store(State::Running);
    // Network/JSON work in a lower-priority task. All SD/SPI and UI stay on main.
    if (xTaskCreate(task,"node-directory",24576,this,0,nullptr) != pdPASS) { fail(Error::Memory); return false; }
    return true;
}
void NodeDirectoryDownload::task(void* context) {
    auto* self = static_cast<NodeDirectoryDownload*>(context);
    const bool ok = self->fetch();
    if (ok) self->_state.store(State::Ready);
    else { self->_pending.release(); self->_state.store(State::Failed); }
    vTaskDelete(nullptr);
}
bool NodeDirectoryDownload::fetch() {
    auto stop = [this](Error error) { _error.store(error); return false; };
    if (!_pending.allocate()) return stop(Error::Memory);
    constexpr unsigned PAGE = 50, MAX_PAGES = 100;
    for (unsigned page = 0; page < MAX_PAGES; ++page) {
        if (_cancel.load()) return stop(Error::Cancelled);
        WiFiClientSecure tls;
        tls.setCACertBundle(rootca_crt_bundle_start);
        tls.setHandshakeTimeout(12);
        HTTPClient http;
        http.setConnectTimeout(10000); http.setTimeout(12000);
        http.useHTTP10(true); // avoid keep-alive surprises; chunked still de-framed by HTTPClient
        http.setUserAgent("MCLite-node-directory/1");
        const String url = String("https://map.carinthiamesh.com/api/nodes?limit=50&offset=") + String(page * PAGE);
        if (!http.begin(tls,url)) return stop(Error::Network);
        http.addHeader("Accept","application/json");
        http.addHeader("Accept-Encoding","identity");
        const int code = http.GET();
        if (code != HTTP_CODE_OK) { http.end(); return stop(Error::Network); }
        if (http.getSize() > int(BoundedBody::CAP)) { http.end(); return stop(Error::Limit); }
        BoundedBody body;
        if (!body.text.reserve(BoundedBody::CAP)) { http.end(); return stop(Error::Memory); }
        const int bytes = http.writeToStream(&body);
        http.end();
        if (_cancel.load()) return stop(Error::Cancelled);
        if (body.overflow) return stop(Error::Limit);
        if (bytes <= 0 || size_t(bytes) != body.text.length()) return stop(Error::Network);
        bool hasMore = false;
        const auto parsed = appendDirectoryPage(body.text.c_str(),body.text.length(),_pending,hasMore);
        if (parsed != DirectoryPageResult::Ok) return stop(parsed == DirectoryPageResult::Memory ? Error::Memory :
                                                          parsed == DirectoryPageResult::Limit ? Error::Limit : Error::Format);
        _progress.store(_pending.count());
        if (!hasMore) return _pending.count() ? true : stop(Error::Format);
        // Empty filtered pages may still have later pages. Respect has_more, not total/length.
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    return stop(Error::Limit); // never silently install a truncated directory
}
void NodeDirectoryDownload::poll() {
    State current = state();
    if (current != State::Ready && current != State::Saving) return;
    auto& directory = NodeDirectory::instance();
    if (_cancel.load()) {
        if (current == State::Saving) directory.cancelInstall();
        _pending.release(); fail(Error::Cancelled); return;
    }
    if (current == State::Ready) {
        if (!directory.beginInstall(_pending)) { _pending.release(); fail(Error::Storage); return; }
        _saved.store(_pending.count()); // candidate size; exposed as saved only after promotion
        _state.store(State::Saving);
    }
    const auto result = directory.saveStep();
    if (result == NodeDirectory::SaveResult::Busy) return;
    _pending.release();
    if (result == NodeDirectory::SaveResult::Failed) { fail(Error::Storage); return; }
    _state.store(State::Saved);
}
}
