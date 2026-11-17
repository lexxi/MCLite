#pragma once
#include <atomic>
#include <Arduino.h>
#include "../util/DirectoryTable.h"
namespace mclite {
class NodeDirectoryDownload {
public:
    enum class State : uint8_t { Idle, Running, Ready, Saving, Saved, Failed };
    enum class Error : uint8_t { None, Offline, Memory, Network, Format, Limit, Storage, Cancelled };
    static NodeDirectoryDownload& instance();
    enum class Kind : uint8_t { Nodes, German };
    bool start(Kind kind = Kind::Nodes);
    Kind kind() const { return _kind.load(); }
    bool languageSaved() const { return _languageSaved.load(); }
    void cancel() { _cancel.store(true); }
    void poll(); // main-loop only: SD promotion; worker never touches shared SPI or LVGL
    State state() const { return _state.load(); }
    Error error() const { return _error.load(); }
    uint32_t progress() const { return _progress.load(); }
    uint32_t savedCount() const { return _saved.load(); }
    bool busy() const { State s = state(); return s == State::Running || s == State::Ready || s == State::Saving; }
private:
    static void task(void* self);
    bool fetch();
    bool fetchLanguage();
    void fail(Error error) { _error.store(error); _state.store(State::Failed); }
    std::atomic<State> _state{State::Idle};
    std::atomic<Error> _error{Error::None};
    std::atomic<bool> _cancel{false};
    std::atomic<uint32_t> _progress{0}, _saved{0};
    DirectoryTable _pending;
    std::atomic<Kind> _kind{Kind::Nodes};
    std::atomic<bool> _languageSaved{false};
    String _language;
    int _minimumLanguageVersion = 0;
};
}
