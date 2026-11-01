#include <unity.h>
#include <limits>
#include "util/radar.h"
using namespace mclite::radar;

void test_bearings_follow_north_east_south_west() {
    TEST_ASSERT_FLOAT_WITHIN(0.001,0,bearingDeg(0,0,1,0));
    TEST_ASSERT_FLOAT_WITHIN(0.001,90,bearingDeg(0,0,0,1));
    TEST_ASSERT_FLOAT_WITHIN(0.001,180,bearingDeg(0,0,-1,0));
    TEST_ASSERT_FLOAT_WITHIN(0.001,270,bearingDeg(0,0,0,-1));
}
void test_distance_handles_dateline_and_antipodes() {
    TEST_ASSERT_FLOAT_WITHIN(0.001,0,distanceKm(46.9,13.5,46.9,13.5));
    TEST_ASSERT_FLOAT_WITHIN(0.1,111.195,distanceKm(0,0,0,1));
    TEST_ASSERT_FLOAT_WITHIN(0.1,222.390,distanceKm(0,179,0,-179));
    TEST_ASSERT_FLOAT_WITHIN(0.1,20015.114,distanceKm(0,0,0,180));
}
void test_bad_coordinates_are_rejected() {
    TEST_ASSERT_TRUE(validPosition(0,0)); // valid geography; caller checks advert.hasGps
    TEST_ASSERT_TRUE(validPosition(-90,180));
    TEST_ASSERT_FALSE(validPosition(91,0));
    TEST_ASSERT_FALSE(validPosition(0,-181));
    TEST_ASSERT_FALSE(validPosition(std::numeric_limits<double>::quiet_NaN(),0));
    TEST_ASSERT_FALSE(validPosition(0,std::numeric_limits<double>::infinity()));
}
void test_hop_rings_are_topology_not_distance() {
    TEST_ASSERT_FLOAT_WITHIN(0.001,0.25,hopRadius(0));
    TEST_ASSERT_FLOAT_WITHIN(0.001,0.5,hopRadius(1));
    TEST_ASSERT_FLOAT_WITHIN(0.001,0.75,hopRadius(2));
    TEST_ASSERT_FLOAT_WITHIN(0.001,1,hopRadius(3));
    TEST_ASSERT_FLOAT_WITHIN(0.001,1,hopRadius(63));
}
void test_age_fades_across_millis_wrap() {
    TEST_ASSERT_EQUAL_UINT8(255,ageOpacity(1500,1000));
    TEST_ASSERT_EQUAL_UINT8(220,ageOpacity(5000,0));
    TEST_ASSERT_EQUAL_UINT8(150,ageOpacity(700000,0));
    TEST_ASSERT_EQUAL_UINT8(80,ageOpacity(4000000,0));
    TEST_ASSERT_EQUAL_UINT8(255,ageOpacity(100,UINT32_MAX-100));
}
void test_unknown_bearing_is_stable_and_bounded() {
    uint8_t key[32]={1,2,3};
    double a=stableBearing(key);
    TEST_ASSERT_TRUE(a>=0 && a<360);
    TEST_ASSERT_EQUAL_FLOAT(a,stableBearing(key));
    key[31]=42;
    TEST_ASSERT_TRUE(a != stableBearing(key));
}
void setUp() {}
void tearDown() {}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_bearings_follow_north_east_south_west);
    RUN_TEST(test_distance_handles_dateline_and_antipodes);
    RUN_TEST(test_bad_coordinates_are_rejected);
    RUN_TEST(test_hop_rings_are_topology_not_distance);
    RUN_TEST(test_age_fades_across_millis_wrap);
    RUN_TEST(test_unknown_bearing_is_stable_and_bounded);
    return UNITY_END();
}
