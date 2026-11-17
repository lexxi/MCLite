#pragma once
#include "../util/DirectoryTable.h"
#include <ArduinoJson.h>
#include <SD.h>
namespace mclite {
class NodeDirectory {
public:
    static constexpr const char* PATH = "/mclite/nodes.jsonl";
    static NodeDirectory& instance();
    bool load();  // boot: live file, else backup; invalid input never partially replaces RAM
    enum class SaveResult { Busy, Done, Failed };
    bool beginInstall(DirectoryTable& downloaded);
    SaveResult saveStep(); // main loop only: bounded batches, then atomic promotion
    void cancelInstall();
    const DirectoryTable& table() const { return _table; }
    static bool decode(JsonObjectConst object, DirectoryNode& node);
private:
    DirectoryTable _table;
    DirectoryTable* _installing = nullptr;
    File _file;
    size_t _cursor = 0;
    bool read(const char* path, DirectoryTable& table);
};
}
