#pragma once
#include "DirectoryTable.h"
#include <ArduinoJson.h>
#include <initializer_list>
namespace mclite {
inline bool decodeDirectoryNode(JsonObjectConst object, DirectoryNode& node) {
    node = DirectoryNode{};
    if (object.isNull() || !DirectoryTable::decodeKey(object["public_key"] | "", node.key)) return false;
    DirectoryTable::copyName(node.name, sizeof node.name, object["name"] | "");
    DirectoryTable::copyName(node.lastSeen, sizeof node.lastSeen, object["last_seen"] | "");
    node.type = DirectoryTable::role(object["role"] | "");
    if (object["lat"].is<double>() && object["lon"].is<double>()) {
        const double lat = object["lat"].as<double>(), lon = object["lon"].as<double>();
        if (!DirectoryTable::validPosition(lat,lon)) return false;
        node.latE6 = int32_t(std::lround(lat * 1e6));
        node.lonE6 = int32_t(std::lround(lon * 1e6));
        node.hasPosition = true;
    } else if (!object["lat"].isNull() || !object["lon"].isNull()) return false;
    return true;
}
enum class DirectoryPageResult { Ok, Format, Limit, Memory };
inline DirectoryPageResult appendDirectoryPage(const char* payload, size_t length,
                                               DirectoryTable& table, bool& hasMore) {
    JsonDocument filter;
    filter["has_more"] = true;
    for (const char* field : {"public_key","name","role","lat","lon","last_seen"})
        filter["nodes"][0][field] = true;
    JsonDocument doc;
    const auto error = deserializeJson(doc,payload,length,DeserializationOption::Filter(filter));
    if (error) return error == DeserializationError::NoMemory ? DirectoryPageResult::Memory : DirectoryPageResult::Format;
    if (!doc["has_more"].is<bool>() || !doc["nodes"].is<JsonArray>()) return DirectoryPageResult::Format;
    JsonArrayConst nodes = doc["nodes"].as<JsonArrayConst>();
    if (nodes.size() > 50) return DirectoryPageResult::Format;
    for (JsonObjectConst object : nodes) {
        DirectoryNode node;
        if (!decodeDirectoryNode(object,node)) return DirectoryPageResult::Format;
        if (!table.upsert(node)) return DirectoryPageResult::Limit;
    }
    hasMore = doc["has_more"].as<bool>();
    return DirectoryPageResult::Ok;
}
}
