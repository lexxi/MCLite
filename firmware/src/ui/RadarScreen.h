#pragma once
#include <lvgl.h>
#include <cstdint>
#include "../storage/HeardAdvertCache.h"

namespace mclite {
class RadarScreen {
public:
    void create(lv_obj_t* parent);
    void show();
    void hide();
    void tick();
private:
    struct Dot { uint8_t key[32]; int16_t x, y; double km, bearing; };
    lv_obj_t *_screen=nullptr, *_plot=nullptr, *_info=nullptr, *_modeBtn=nullptr;
    lv_obj_t *_modeLabel=nullptr, *_backBtn=nullptr, *_prevBtn=nullptr, *_nextBtn=nullptr, *_detailBtn=nullptr;
    Dot _dots[HEARD_ADVERT_CAP] = {};
    int _count=0, _selected=0, _radius=0, _cx=0, _cy=0;
    uint32_t _lastRefresh=0;
    bool _geo=false, _autoMode=true, _haveOwn=false, _lastKnown=false, _pending=false;
    double _rangeKm=1;
    uint8_t _pendingKey[32] = {};
    void refresh();
    void updateInfo();
    void openSelected();
    static void drawCb(lv_event_t* e);
    static void clickCb(lv_event_t* e);
    static void backCb(lv_event_t* e);
    static void modeCb(lv_event_t* e);
    static void selectCb(lv_event_t* e);
    static void detailsCb(lv_event_t* e);
};
}
