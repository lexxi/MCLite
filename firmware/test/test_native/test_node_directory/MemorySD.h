#pragma once
#include <map>
#include <string>
#include <memory>
#include <cstring>
#include <algorithm>
#define FILE_READ "r"
#define FILE_WRITE "w"
static long directoryWriteBudget = -1;
class File {
public:
    File() = default;
    File(std::shared_ptr<std::string> data) : _data(data) {}
    operator bool() const { return bool(_data); }
    size_t size() const { return _data ? _data->size() : 0; }
    int available() const { return _data && _position < _data->size(); }
    int read() { return available() ? static_cast<unsigned char>((*_data)[_position++]) : -1; }
    size_t write(const uint8_t* bytes,size_t count) {
        if (!_data) return 0;
        if (directoryWriteBudget >= 0) count = std::min(count,size_t(directoryWriteBudget));
        if (directoryWriteBudget >= 0) directoryWriteBudget -= count;
        _data->append(reinterpret_cast<const char*>(bytes),count);
        return count;
    }
    void flush() {}
    void close() { _data.reset(); }
private:
    std::shared_ptr<std::string> _data;
    size_t _position = 0;
};
class DirectoryMemorySD {
public:
    std::map<std::string,std::shared_ptr<std::string>> files;
    bool failPromotion = false;
    File open(const char* path,const char* mode = FILE_READ) {
        if (!strcmp(mode,FILE_WRITE)) files[path] = std::make_shared<std::string>();
        auto it = files.find(path);
        return it == files.end() ? File() : File(it->second);
    }
    bool exists(const char* path) { return files.count(path); }
    bool remove(const char* path) { return files.erase(path); }
    bool rename(const char* source,const char* dest) {
        if (failPromotion && std::string(source).find(".tmp") != std::string::npos) return false;
        auto it = files.find(source);
        if (it == files.end() || exists(dest)) return false;
        files[dest] = it->second; files.erase(it); return true;
    }
};
static DirectoryMemorySD SD;
