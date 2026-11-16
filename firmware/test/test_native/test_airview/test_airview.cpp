#ifdef AIRVIEW_STANDALONE
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
#include <cstring>
#include <limits>
#include "util/AirViewData.h"
using namespace mclite;
void setUp() {}
void tearDown() {}
void test_disabled_collection_and_reopen() {
    AirViewData data;
    TEST_ASSERT_FALSE(data.sampleDue(0));
    data.sample(0,true,-100); data.packet(0,4,20,-100,3);
    TEST_ASSERT_EQUAL_UINT(0,data.sampleCount());
    TEST_ASSERT_EQUAL_UINT(0,data.eventCount());
    data.begin(); data.sample(1,true,-100); data.packet(1,4,20,-100,3);
    TEST_ASSERT_EQUAL_UINT(1,data.rxCount);
    data.setActive(false); data.sample(2,true,-90); data.packet(2,5,21,-90,5);
    TEST_ASSERT_EQUAL_UINT(1,data.sampleCount());
    TEST_ASSERT_EQUAL_UINT(1,data.eventCount());
    data.begin();
    TEST_ASSERT_EQUAL_UINT(0,data.sampleCount());
    TEST_ASSERT_EQUAL_UINT(0,data.eventCount());
    TEST_ASSERT_EQUAL_UINT(0,data.rxCount);
}
void test_sample_timing_wrap_and_pause() {
    AirViewData data; data.begin();
    const uint32_t time = UINT32_MAX - 49;
    TEST_ASSERT_TRUE(data.sampleDue(time));
    TEST_ASSERT_FALSE(data.sampleDue(49)); // 99 ms across wrap
    TEST_ASSERT_TRUE(data.sampleDue(50));
    TEST_ASSERT_FALSE(data.sampleDue(50));
    data.setActive(false);
    TEST_ASSERT_FALSE(data.sampleDue(5000));
    data.setActive(true);
    TEST_ASSERT_TRUE(data.sampleDue(5001));
    TEST_ASSERT_FALSE(data.sampleDue(5001)); // no catch-up burst
}
void test_sample_ring_keeps_latest_in_order() {
    AirViewData data; data.begin();
    for (unsigned i=0; i<200; ++i) data.sample(i*100,true,-100);
    TEST_ASSERT_EQUAL_UINT(AirViewData::SAMPLE_CAP,data.sampleCount());
    TEST_ASSERT_EQUAL_UINT(10400,data.sampleAt(0).time);
    TEST_ASSERT_EQUAL_UINT(19900,data.sampleAt(95).time);
}
void test_tx_and_invalid_levels_are_gaps() {
    AirViewData data; data.begin();
    data.sample(0,false,-90);
    data.sample(100,true,0);
    data.sample(200,true,std::numeric_limits<float>::quiet_NaN());
    data.sample(300,true,-151);
    data.sample(400,true,-120.5);
    for (unsigned i=0;i<4;++i) TEST_ASSERT_FALSE(data.sampleAt(i).valid);
    TEST_ASSERT_TRUE(data.sampleAt(4).valid);
}
void test_packet_ring_and_separate_rx_tx_fail_counts() {
    AirViewData data; data.begin();
    for (unsigned i=0; i<30; ++i) data.packet(i,4,40,-98,7.5);
    data.packet(30,5,50,0,0,true);
    data.packet(31,2,60,0,0,true,true);
    TEST_ASSERT_EQUAL_UINT(30,data.rxCount);
    TEST_ASSERT_EQUAL_UINT(30,data.types[4]);
    TEST_ASSERT_EQUAL_UINT(0,data.types[5]);
    TEST_ASSERT_EQUAL_UINT(1,data.txCount);
    TEST_ASSERT_EQUAL_UINT(1,data.failCount);
    TEST_ASSERT_EQUAL_UINT(AirViewData::EVENT_CAP,data.eventCount());
    TEST_ASSERT_EQUAL_UINT(8,data.eventAt(0).time);
    TEST_ASSERT_TRUE(data.eventAt(23).failed);
    TEST_ASSERT_TRUE(data.eventAt(23).tx);
    TEST_ASSERT_EQUAL_UINT(60,data.eventAt(23).length);
}
void test_type_names_and_settings_survive_reset() {
    TEST_ASSERT_EQUAL_STRING("ADV",AirViewData::typeName(4));
    TEST_ASSERT_EQUAL_STRING("CH",AirViewData::typeName(5));
    TEST_ASSERT_EQUAL_STRING("ACK",AirViewData::typeName(3));
    TEST_ASSERT_EQUAL_STRING("?",AirViewData::typeName(12));
    AirViewData data;
    data.radio.frequency=869.618f; data.radio.sf=8;
    data.begin();
    TEST_ASSERT_TRUE(data.radio.frequency>869 && data.radio.frequency<870);
    TEST_ASSERT_EQUAL_UINT(8,data.radio.sf);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_disabled_collection_and_reopen);
    RUN_TEST(test_sample_timing_wrap_and_pause);
    RUN_TEST(test_sample_ring_keeps_latest_in_order);
    RUN_TEST(test_tx_and_invalid_levels_are_gaps);
    RUN_TEST(test_packet_ring_and_separate_rx_tx_fail_counts);
    RUN_TEST(test_type_names_and_settings_survive_reset);
    return UNITY_END();
}
