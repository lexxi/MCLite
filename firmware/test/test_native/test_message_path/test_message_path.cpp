#ifdef MESSAGE_PATH_STANDALONE
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
#include <limits>
#include "util/MessagePath.h"
#include "util/PathLine.h"
using namespace mclite;

void setUp() {}
void tearDown() {}
void test_direct_is_distinct_from_legacy() {
    MessagePath legacy;
    TEST_ASSERT_FALSE(legacy.valid(0));
    auto direct = MessagePath::capture(nullptr, 1, 0);
    TEST_ASSERT_TRUE(direct.valid(0));
    TEST_ASSERT_FALSE(direct.valid(1));
}
void test_capture_limits_and_roundtrip() {
    uint8_t bytes[64];
    for (unsigned i = 0; i < sizeof(bytes); ++i) bytes[i] = i;
    for (uint8_t width = 1; width <= 4; ++width) {
        uint8_t hops = width == 1 ? 63 : 64 / width;
        auto path = MessagePath::capture(bytes, width, hops);
        TEST_ASSERT_TRUE(path.valid(hops));
        MessagePath restored;
        restored.hashSize = path.hashSize;
        restored.hex = path.hex.c_str();
        TEST_ASSERT_TRUE(restored.valid(hops));
        TEST_ASSERT_TRUE(restored.matches(0, bytes));
        TEST_ASSERT_TRUE(restored.matches(hops - 1, bytes + (hops - 1) * width));
        TEST_ASSERT_FALSE(restored.matches(hops, bytes));
    }
    TEST_ASSERT_FALSE(MessagePath::capture(bytes, 4, 17).valid(17));
    TEST_ASSERT_FALSE(MessagePath::capture(bytes, 0, 1).valid(1));
    TEST_ASSERT_FALSE(MessagePath::capture(bytes, 5, 1).valid(1));
    TEST_ASSERT_FALSE(MessagePath::capture(nullptr, 1, 1).valid(1));
    TEST_ASSERT_FALSE(MessagePath::capture(bytes, 1, 64).valid(64));
}
void test_corrupt_history_is_rejected() {
    MessagePath path;
    path.hashSize = 2;
    path.hex = "Ab01";
    TEST_ASSERT_TRUE(path.valid(1));
    path.hex = "Ag01";
    TEST_ASSERT_FALSE(path.valid(1));
    path.hex = "ab0";
    TEST_ASSERT_FALSE(path.valid(1));
    path.hex = "ab0100";
    TEST_ASSERT_FALSE(path.valid(1));
}
void test_collisions_are_not_resolved_by_position() {
    uint8_t first[32] = {0xab, 0x01}, second[32] = {0xab, 0x02};
    uint8_t duplicate[32]; memcpy(duplicate, first, 32);
    auto path = MessagePath::capture(first, 1, 1);
    std::vector<const uint8_t*> keys;
    TEST_ASSERT_TRUE(addMessagePathCandidate(path, 0, keys, first));
    TEST_ASSERT_FALSE(addMessagePathCandidate(path, 0, keys, duplicate));
    TEST_ASSERT_EQUAL_UINT(1, keys.size());
    // A second identity counts even without GPS. It makes a 1-byte hash ambiguous.
    TEST_ASSERT_TRUE(addMessagePathCandidate(path, 0, keys, second));
    TEST_ASSERT_EQUAL_UINT(2, keys.size());
    keys.clear();
    path = MessagePath::capture(first, 2, 1);
    TEST_ASSERT_TRUE(addMessagePathCandidate(path, 0, keys, first));
    TEST_ASSERT_FALSE(addMessagePathCandidate(path, 0, keys, second));
    TEST_ASSERT_EQUAL_UINT(1, keys.size());
}
void test_map_line_clipping() {
    double x0 = -1000000, y0 = 120, x1 = 1000000, y1 = 120;
    TEST_ASSERT_TRUE(clipPathLine(x0,y0,x1,y1,319,239));
    TEST_ASSERT_TRUE(std::abs(x0) < 0.00001 && std::abs(x1-319) < 0.00001);
    x0 = -20; x1 = -10; y0 = 0; y1 = 239;
    TEST_ASSERT_FALSE(clipPathLine(x0,y0,x1,y1,319,239));
    x0 = x1 = 100; y0 = -100000; y1 = 100000;
    TEST_ASSERT_TRUE(clipPathLine(x0,y0,x1,y1,319,239));
    TEST_ASSERT_TRUE(std::abs(y0) < 0.00001 && std::abs(y1-239) < 0.00001);
    x0 = std::numeric_limits<double>::quiet_NaN();
    TEST_ASSERT_FALSE(clipPathLine(x0,y0,x1,y1,319,239));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_direct_is_distinct_from_legacy);
    RUN_TEST(test_capture_limits_and_roundtrip);
    RUN_TEST(test_corrupt_history_is_rejected);
    RUN_TEST(test_collisions_are_not_resolved_by_position);
    RUN_TEST(test_map_line_clipping);
    return UNITY_END();
}
