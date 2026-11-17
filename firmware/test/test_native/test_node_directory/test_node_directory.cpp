#include <Arduino.h>
#define NODE_DIRECTORY_STORAGE_TEST
#ifdef NODE_DIRECTORY_STANDALONE
#include <cassert>
#define TEST_ASSERT_TRUE(x) assert(x)
#define TEST_ASSERT_FALSE(x) assert(!(x))
#define TEST_ASSERT_EQUAL_UINT(x,y) assert((x)==(y))
#define TEST_ASSERT_EQUAL_STRING(x,y) assert(strcmp((x),(y))==0)
#define UNITY_BEGIN() 0
#define UNITY_END() 0
#define RUN_TEST(x) x()
#else
#include <unity.h>
#endif
#include <string>
#include "util/DirectoryJson.h"
#include "util/MessagePath.h"
#include "storage/NodeDirectory.cpp"
namespace mclite {
SDCard& SDCard::instance() { static SDCard card; card._mounted = true; return card; }
bool SDCard::mkdir(const char*) { return true; }
File SDCard::openRaw(const char* path) { return SD.open(path); }
}
using namespace mclite;
void setUp() {}
void tearDown() {}
static const char* KEY = "0e6cebecf336e0b7de13ee8be9b4a5eb7050620ba7866563fcc20348310c2973";
static DirectoryNode node(unsigned number) {
    DirectoryNode n = {};
    n.key[0] = number & 255; n.key[1] = number >> 8;
    n.type = 2; n.hasPosition = true; n.latE6 = 46666591; n.lonE6 = 14279462;
    strcpy(n.name,"Emmersdorf");
    return n;
}
static std::string page(const char* key = KEY,const char* lat = "46.666591",const char* role = "repeater",const char* more = "false") {
    return std::string("{\"has_more\":") + more + ",\"total\":1,\"nodes\":[{\"public_key\":\"" + key + "\",\"name\":\"AT-K-Test\",\"role\":\"" + role + "\",\"lat\":" + lat + ",\"lon\":14.279462}]}";
}
void test_full_keys_only_and_validation() {
    uint8_t key[32];
    TEST_ASSERT_TRUE(DirectoryTable::decodeKey(KEY,key));
    TEST_ASSERT_FALSE(DirectoryTable::decodeKey("0e",key));
    TEST_ASSERT_FALSE(DirectoryTable::decodeKey("0000000000000000000000000000000000000000000000000000000000000000",key));
    std::string invalid(KEY); invalid[63] = 'g';
    TEST_ASSERT_FALSE(DirectoryTable::decodeKey(invalid.c_str(),key));
    TEST_ASSERT_TRUE(DirectoryTable::validPosition(0,0));
    TEST_ASSERT_FALSE(DirectoryTable::validPosition(91,14));
}
void test_parse_real_schema_and_dedup() {
    DirectoryTable table; TEST_ASSERT_TRUE(table.allocate()); bool more = false;
    auto text = page();
    TEST_ASSERT_TRUE(appendDirectoryPage(text.c_str(),text.size(),table,more) == DirectoryPageResult::Ok);
    TEST_ASSERT_FALSE(more); TEST_ASSERT_EQUAL_UINT(1,table.count());
    TEST_ASSERT_EQUAL_UINT(46666591,table.entries()[0].latE6);
    TEST_ASSERT_EQUAL_UINT(14279462,table.entries()[0].lonE6);
    TEST_ASSERT_TRUE(table.entries()[0].hasPosition);
    TEST_ASSERT_EQUAL_STRING("AT-K-Test",table.entries()[0].name);
    TEST_ASSERT_TRUE(appendDirectoryPage(text.c_str(),text.size(),table,more) == DirectoryPageResult::Ok);
    TEST_ASSERT_EQUAL_UINT(1,table.count());
}
void test_has_more_is_independent_of_total_or_empty_page() {
    DirectoryTable table; TEST_ASSERT_TRUE(table.allocate()); bool more = false;
    const char* text = "{\"nodes\":[],\"total\":0,\"has_more\":true}";
    TEST_ASSERT_TRUE(appendDirectoryPage(text,strlen(text),table,more) == DirectoryPageResult::Ok);
    TEST_ASSERT_TRUE(more);
    text = "{\"nodes\":[]}";
    TEST_ASSERT_TRUE(appendDirectoryPage(text,strlen(text),table,more) == DirectoryPageResult::Format);
}
void test_bad_schema_coordinates_and_keys_fail_closed() {
    DirectoryTable table; TEST_ASSERT_TRUE(table.allocate()); bool more;
    for (const auto& text : {page("0e"),page(KEY,"91"),page(KEY,"\"46.6\""),page(KEY,"null")}) {
        TEST_ASSERT_TRUE(appendDirectoryPage(text.c_str(),text.size(),table,more) == DirectoryPageResult::Format);
    }
    TEST_ASSERT_EQUAL_UINT(0,table.count());
}
void test_no_gps_and_unknown_role_still_count_for_ambiguity() {
    DirectoryTable table; TEST_ASSERT_TRUE(table.allocate());
    JsonDocument doc; DirectoryNode first,second;
    doc["public_key"] = KEY; doc["role"] = "repeater";
    TEST_ASSERT_TRUE(decodeDirectoryNode(doc.as<JsonObjectConst>(),first));
    TEST_ASSERT_FALSE(first.hasPosition);
    TEST_ASSERT_TRUE(table.upsert(first));
    second = first; second.key[31] ^= 1; second.type = DirectoryTable::role("future-role");
    TEST_ASSERT_TRUE(table.upsert(second));
    auto path = MessagePath::capture(first.key,1,1);
    std::vector<const uint8_t*> keys;
    for (size_t i=0;i<table.count();++i) addMessagePathCandidate(path,0,keys,table.entries()[i].key);
    TEST_ASSERT_EQUAL_UINT(2,keys.size());
}
void test_capacity_and_replace_by_full_key() {
    DirectoryTable table; TEST_ASSERT_TRUE(table.allocate());
    for (unsigned i=1;i<=DirectoryTable::CAP;++i) TEST_ASSERT_TRUE(table.upsert(node(i)));
    TEST_ASSERT_FALSE(table.upsert(node(DirectoryTable::CAP+1)));
    auto replacement = node(10); strcpy(replacement.name,"Updated");
    TEST_ASSERT_TRUE(table.upsert(replacement));
    TEST_ASSERT_EQUAL_STRING("Updated",table.find(replacement.key)->name);
    TEST_ASSERT_EQUAL_UINT(DirectoryTable::CAP,table.count());
}
void test_safe_ui_names() {
    char name[5];
    DirectoryTable::copyName(name,sizeof name,"abc\xc3\xa4");
    TEST_ASSERT_EQUAL_STRING("abc",name);
    DirectoryTable::copyName(name,sizeof name,"a\nb\t");
    TEST_ASSERT_EQUAL_STRING("a b ",name);
}
static void save(DirectoryTable& table) {
    auto& directory = NodeDirectory::instance();
    TEST_ASSERT_TRUE(directory.beginInstall(table));
    auto result = NodeDirectory::SaveResult::Busy;
    while (result == NodeDirectory::SaveResult::Busy) result = directory.saveStep();
    TEST_ASSERT_TRUE(result == NodeDirectory::SaveResult::Done);
}
void test_atomic_save_cancellation_short_write_and_backup() {
    auto& directory = NodeDirectory::instance();
    SD.files.clear(); directoryWriteBudget = -1;
    DirectoryTable initial; TEST_ASSERT_TRUE(initial.allocate()); TEST_ASSERT_TRUE(initial.upsert(node(1)));
    save(initial);
    const auto original = *SD.files[NodeDirectory::PATH];
    DirectoryTable candidate; TEST_ASSERT_TRUE(candidate.allocate());
    for (unsigned i=2;i<=21;++i) TEST_ASSERT_TRUE(candidate.upsert(node(i)));
    TEST_ASSERT_TRUE(directory.beginInstall(candidate));
    TEST_ASSERT_TRUE(directory.saveStep() == NodeDirectory::SaveResult::Busy);
    TEST_ASSERT_EQUAL_UINT(1,directory.table().count());
    directory.cancelInstall();
    TEST_ASSERT_TRUE(original == *SD.files[NodeDirectory::PATH]);
    TEST_ASSERT_TRUE(directory.beginInstall(candidate));
    directoryWriteBudget = 1;
    TEST_ASSERT_TRUE(directory.saveStep() == NodeDirectory::SaveResult::Failed);
    directoryWriteBudget = -1;
    TEST_ASSERT_TRUE(original == *SD.files[NodeDirectory::PATH]);
    SD.failPromotion = true;
    TEST_ASSERT_TRUE(directory.beginInstall(candidate));
    while (directory.saveStep() == NodeDirectory::SaveResult::Busy) {}
    SD.failPromotion = false;
    TEST_ASSERT_TRUE(original == *SD.files[NodeDirectory::PATH]);
    TEST_ASSERT_EQUAL_UINT(1,directory.table().count());
    save(candidate);
    TEST_ASSERT_EQUAL_UINT(20,directory.table().count());
    TEST_ASSERT_TRUE(SD.exists("/mclite/nodes.jsonl.bak"));
    // Interrupted promotion / damaged active file: reload the old complete backup.
    SD.files[NodeDirectory::PATH] = std::make_shared<std::string>("broken");
    TEST_ASSERT_TRUE(directory.load());
    TEST_ASSERT_EQUAL_UINT(1,directory.table().count());
    TEST_ASSERT_TRUE(original == *SD.files[NodeDirectory::PATH]);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_full_keys_only_and_validation);
    RUN_TEST(test_parse_real_schema_and_dedup);
    RUN_TEST(test_has_more_is_independent_of_total_or_empty_page);
    RUN_TEST(test_bad_schema_coordinates_and_keys_fail_closed);
    RUN_TEST(test_no_gps_and_unknown_role_still_count_for_ambiguity);
    RUN_TEST(test_capacity_and_replace_by_full_key);
    RUN_TEST(test_safe_ui_names);
    RUN_TEST(test_atomic_save_cancellation_short_write_and_backup);
    return UNITY_END();
}
