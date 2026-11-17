#include <unity.h>
#include "net/WiFiManager.cpp"
using namespace mclite;
void setUp() { WiFi.rows.clear(); WiFi.calls.clear(); WiFi.result=0; WiFi.driverStatus=0; WiFi.driverCount=0; WiFi.modeSucceeds=true; }
void tearDown() {}
void test_driver_failure_is_not_an_empty_success() {
    setUp(); WiFi.result=-2; ScannedNetwork out[4];
    auto& manager=WiFiManager::instance();
    TEST_ASSERT_EQUAL_INT(0,manager.scan(out,4));
    TEST_ASSERT_EQUAL_INT(-2,manager.lastScanResult());
    TEST_ASSERT_TRUE(manager.scanFailed());
}
void test_mode_start_failure_is_reported_without_scan() {
    setUp(); WiFi.modeSucceeds=false; ScannedNetwork out[4];
    auto& manager=WiFiManager::instance(); manager.scan(out,4);
    TEST_ASSERT_EQUAL_INT(-3,manager.lastScanResult());
    TEST_ASSERT_TRUE(manager.scanFailed());
    TEST_ASSERT_TRUE(std::find(WiFi.calls.begin(),WiFi.calls.end(),"scan")==WiFi.calls.end());
}
void test_empty_success_and_missing_result_buffer_are_distinct() {
    setUp(); ScannedNetwork out[4]; auto& manager=WiFiManager::instance();
    manager.scan(out,4); TEST_ASSERT_FALSE(manager.scanFailed());
    TEST_ASSERT_EQUAL_INT(0,manager.lastScanAccessPoints());
    WiFi.driverCount=5;
    manager.scan(out,4); TEST_ASSERT_TRUE(manager.scanFailed());
    WiFi.driverCount=0; WiFi.driverStatus=1;
    manager.scan(out,4); TEST_ASSERT_TRUE(manager.scanFailed());
}
void test_clean_start_and_no_credential_erasure() {
    setUp(); ScannedNetwork out[4]; WiFiManager::instance().scan(out,4);
    TEST_ASSERT_TRUE(std::find(WiFi.calls.begin(),WiFi.calls.end(),"off") < std::find(WiFi.calls.begin(),WiFi.calls.end(),"sta"));
    TEST_ASSERT_TRUE(std::find(WiFi.calls.begin(),WiFi.calls.end(),"sta") < std::find(WiFi.calls.begin(),WiFi.calls.end(),"scan"));
    TEST_ASSERT_TRUE(std::find(WiFi.calls.begin(),WiFi.calls.end(),"disconnect")==WiFi.calls.end());
    TEST_ASSERT_TRUE(WiFi.hidden);
}
void test_dedup_strongest_limit_and_hidden_names() {
    setUp(); WiFi.rows={{"A",-80,3},{"A",-45,0},{"B",-65,3},{"",-20,3},{"C",-30,3}};
    WiFi.result=WiFi.rows.size(); WiFi.driverCount=WiFi.result;
    ScannedNetwork out[2]; auto& manager=WiFiManager::instance();
    TEST_ASSERT_EQUAL_INT(2,manager.scan(out,2));
    TEST_ASSERT_EQUAL_STRING("C",out[0].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("A",out[1].ssid.c_str());
    TEST_ASSERT_TRUE(out[1].open);
    TEST_ASSERT_FALSE(manager.scanFailed());
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_driver_failure_is_not_an_empty_success);
    RUN_TEST(test_mode_start_failure_is_reported_without_scan);
    RUN_TEST(test_empty_success_and_missing_result_buffer_are_distinct);
    RUN_TEST(test_clean_start_and_no_credential_erasure);
    RUN_TEST(test_dedup_strongest_limit_and_hidden_names);
    return UNITY_END();
}
