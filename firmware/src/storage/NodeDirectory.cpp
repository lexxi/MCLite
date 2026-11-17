#include "NodeDirectory.h"
#include "../util/DirectoryJson.h"
#include "SDCard.h"
#include "../util/log.h"
#include <SD.h>
namespace mclite {
constexpr const char* NodeDirectory::PATH;
NodeDirectory& NodeDirectory::instance() { static NodeDirectory directory; return directory; }
bool NodeDirectory::decode(JsonObjectConst object, DirectoryNode& node) {
    return decodeDirectoryNode(object,node);
}
bool NodeDirectory::read(const char* path, DirectoryTable& table) {
    File file = SDCard::instance().openRaw(path);
    if (!file || file.size() > 2 * 1024 * 1024 || !table.allocate()) return false;
    while (file.available()) {
        // Read bounded lines; a broken/edited file must not grow an unbounded String.
        char line[768]; size_t length = 0;
        while (file.available()) {
            int c = file.read();
            if (c < 0) return false;
            if (c == '\n') break;
            if (length >= sizeof(line) - 1) return false;
            line[length++] = char(c);
        }
        line[length] = 0;
        if (!length || (length == 1 && line[0] == '\r')) continue;
        JsonDocument doc; DirectoryNode node;
        if (deserializeJson(doc,line) || !decode(doc.as<JsonObjectConst>(),node) || !table.upsert(node)) return false;
    }
    return table.count() > 0;
}
bool NodeDirectory::load() {
    if (!SDCard::instance().isMounted()) return false;
    DirectoryTable fresh;
    if (!read(PATH,fresh)) {
        fresh.release();
        if (!read("/mclite/nodes.jsonl.bak",fresh)) return false;
        // Restore the known-good backup after interruption/corruption.
        SD.remove(PATH);
        SD.rename("/mclite/nodes.jsonl.bak",PATH);
    }
    _table.swap(fresh);
    LOGF("[Nodes] loaded %u directory identities\n", unsigned(_table.count()));
    return true;
}
bool NodeDirectory::beginInstall(DirectoryTable& downloaded) {
    if (_installing || !downloaded.count() || !SDCard::instance().isMounted()) return false;
    SDCard::instance().mkdir("/mclite");
    SD.remove("/mclite/nodes.jsonl.tmp");
    _file = SD.open("/mclite/nodes.jsonl.tmp",FILE_WRITE);
    if (!_file) return false;
    _installing = &downloaded; _cursor = 0;
    return true;
}
void NodeDirectory::cancelInstall() {
    _file.close(); _installing = nullptr;
    SD.remove("/mclite/nodes.jsonl.tmp");
}
NodeDirectory::SaveResult NodeDirectory::saveStep() {
    if (!_installing) return SaveResult::Failed;
    // Eight buffered line writes per main-loop pass; LoRa is serviced between batches.
    for (unsigned batch = 0; batch < 8 && _cursor < _installing->count(); ++batch, ++_cursor) {
        const auto& node = _installing->entries()[_cursor];
        char key[65];
        for (size_t j = 0; j < 32; ++j) snprintf(key + j * 2,3,"%02x",node.key[j]);
        JsonDocument doc;
        doc["public_key"] = key; doc["name"] = node.name; doc["last_seen"] = node.lastSeen;
        doc["role"] = node.type == 1 ? "companion" : node.type == 2 ? "repeater" : node.type == 3 ? "room" : node.type == 4 ? "sensor" : "unknown";
        if (node.hasPosition) { doc["lat"] = node.latE6 / 1e6; doc["lon"] = node.lonE6 / 1e6; }
        else { doc["lat"] = nullptr; doc["lon"] = nullptr; }
        char line[512];
        const size_t expected = measureJson(doc);
        if (expected + 1 >= sizeof(line) || serializeJson(doc,line,sizeof(line)) != expected) {
            cancelInstall(); return SaveResult::Failed;
        }
        line[expected] = '\n';
        if (_file.write(reinterpret_cast<const uint8_t*>(line),expected + 1) != expected + 1) {
            cancelInstall(); return SaveResult::Failed;
        }
    }
    if (_cursor < _installing->count()) return SaveResult::Busy;
    _file.flush(); _file.close();
    const char* temp = "/mclite/nodes.jsonl.tmp";
    const char* backup = "/mclite/nodes.jsonl.bak";
    bool rotated = false;
    if (SD.exists(PATH)) {
        if (SD.exists(backup) && !SD.remove(backup)) { cancelInstall(); return SaveResult::Failed; }
        if (!SD.rename(PATH,backup)) { cancelInstall(); return SaveResult::Failed; }
        rotated = true;
    }
    if (!SD.rename(temp,PATH)) {
        if (rotated) SD.rename(backup,PATH);
        cancelInstall(); return SaveResult::Failed;
    }
    _table.swap(*_installing);
    _installing = nullptr;
    return SaveResult::Done;
}
}
