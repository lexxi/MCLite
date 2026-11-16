#pragma once
#include <lvgl.h>
#include <cstdint>

namespace mclite {
class AirViewScreen {
public:
    void create(lv_obj_t* parent);
    void show();
    void hide();
    void tick(bool foreground);
private:
    lv_obj_t *_screen = nullptr, *_plot = nullptr, *_stats = nullptr, *_feed = nullptr;
    lv_obj_t *_back = nullptr, *_reset = nullptr, *_help = nullptr;
    uint32_t _lastRefresh = 0;
    void refresh();
    static void drawCb(lv_event_t* event);
    static void backCb(lv_event_t* event);
    static void resetCb(lv_event_t* event);
    static void helpCb(lv_event_t* event);
};
}
