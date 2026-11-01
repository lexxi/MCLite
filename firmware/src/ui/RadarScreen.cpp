#include "RadarScreen.h"
#include "UIManager.h"
#include "theme.h"
#include "../hal/Display.h"
#include "../hal/GPS.h"
#include "../i18n/I18n.h"
#include "../util/radar.h"
#include "../util/Utf8Trim.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace mclite {
namespace {
lv_color_t nodeColor(uint8_t type) {
    switch (type) {
        case 1: return theme::ACCENT();
        case 2: return theme::OFFGRID_ACCENT();
        case 3: return theme::ROOM_ACCENT();
        case 4: return theme::ONLINE_DOT();
        default: return theme::TEXT_SECONDARY();
    }
}
const HeardAdvert* findNode(const uint8_t* key) {
    auto& c=HeardAdvertCache::instance();
    for (int i=0;i<c.count();++i) if (!memcmp(key,c.entries()[i].pubKey,32)) return &c.entries()[i];
    return nullptr;
}
}
void RadarScreen::create(lv_obj_t* parent) {
    _screen=lv_win_create(parent,theme::CHAT_HEADER_HEIGHT);
    lv_obj_set_size(_screen,Display::width(),Display::height()-theme::STATUS_BAR_HEIGHT-theme::FOOTER_HEIGHT);
    lv_obj_align(_screen,LV_ALIGN_BOTTOM_MID,0,-theme::FOOTER_HEIGHT);
    lv_obj_set_style_bg_color(_screen,theme::BG_PRIMARY(),0);
    lv_obj_set_style_border_width(_screen,0,0);
    lv_obj_set_style_radius(_screen,0,0);
    auto* h=lv_win_get_header(_screen);
    lv_obj_set_style_bg_color(h,theme::BG_STATUS_BAR(),0);
    lv_obj_set_style_pad_all(h,2,0);
    auto button=[this](lv_obj_t* parent,const char* text,int width,lv_event_cb_t cb) {
        auto* b=lv_btn_create(parent);
        lv_obj_set_size(b,width,theme::CHAT_HEADER_HEIGHT-4);
        lv_obj_set_style_pad_all(b,0,0);
        lv_obj_set_style_bg_color(b,theme::BG_INPUT(),0);
        lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,this);
        auto* l=lv_label_create(b);
        lv_label_set_text(l,text); lv_obj_set_style_text_font(l,FONT_BODY,0); lv_obj_center(l);
        return b;
    };
    _backBtn=button(h,LV_SYMBOL_LEFT,theme::BTN_HEADER_BACK_W,backCb);
    auto* title=lv_win_add_title(_screen,"Radar");
    lv_obj_set_style_text_font(title,FONT_HEADING,0);
    lv_obj_set_style_text_color(title,theme::TEXT_PRIMARY(),0);
    _modeBtn=button(h,"",90,modeCb);
    _modeLabel=lv_obj_get_child(_modeBtn,0);
    auto* cont=lv_win_get_content(_screen);
    lv_obj_set_style_bg_color(cont,theme::BG_PRIMARY(),0);
    lv_obj_set_style_pad_all(cont,4,0);
    lv_obj_set_flex_flow(cont,LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(cont,LV_OBJ_FLAG_SCROLLABLE);
    _plot=lv_obj_create(cont);
    lv_obj_remove_style_all(_plot);
    lv_obj_add_flag(_plot,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(_plot,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(_plot,drawCb,LV_EVENT_DRAW_MAIN,this);
    lv_obj_add_event_cb(_plot,clickCb,LV_EVENT_CLICKED,this);
    auto* side=lv_obj_create(cont);
    lv_obj_remove_style_all(side);
    lv_obj_clear_flag(side,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(side,1); lv_obj_set_height(side,LV_PCT(100));
    lv_obj_set_flex_flow(side,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(side,3,0);
    _info=lv_label_create(side);
    lv_obj_set_width(_info,LV_PCT(100));
    lv_obj_set_flex_grow(_info,1);
    lv_obj_set_style_text_font(_info,FONT_BODY,0);
    lv_obj_set_style_text_color(_info,theme::TEXT_PRIMARY(),0);
    lv_label_set_long_mode(_info,LV_LABEL_LONG_WRAP);
    auto* nav=lv_obj_create(side); lv_obj_remove_style_all(nav);
    lv_obj_clear_flag(nav,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(nav,LV_PCT(100),theme::CHAT_HEADER_HEIGHT-4);
    lv_obj_set_flex_flow(nav,LV_FLEX_FLOW_ROW);
    _prevBtn=button(nav,"<",26,selectCb);
    _nextBtn=button(nav,">",26,selectCb);
    _detailBtn=button(nav,"Info",42,detailsCb);
    lv_obj_add_flag(_screen,LV_OBJ_FLAG_HIDDEN);
}
void RadarScreen::show() {
    lv_obj_clear_flag(_screen,LV_OBJ_FLAG_HIDDEN);
    lv_obj_update_layout(_screen);
    auto* cont=lv_win_get_content(_screen);
    const int side=std::min((int)lv_obj_get_content_height(cont), (int)theme::CONTENT_WIDTH*54/100);
    lv_obj_set_size(_plot,side,side);
    _cx=_cy=side/2; _radius=std::max(10,side/2-16);
    auto* grp=lv_group_get_default();
    if(grp) { for(auto* b:{_backBtn,_modeBtn,_prevBtn,_nextBtn,_detailBtn}) lv_group_add_obj(grp,b); lv_group_focus_obj(_backBtn); }
    refresh();
    _lastRefresh=millis();
}
void RadarScreen::hide() {
    if (!_screen) return;
    for(auto* b:{_backBtn,_modeBtn,_prevBtn,_nextBtn,_detailBtn}) lv_group_remove_obj(b);
    lv_obj_add_flag(_screen,LV_OBJ_FLAG_HIDDEN);
}
void RadarScreen::refresh() {
    uint8_t selected[32]={};
    const bool hadSelection=_count>0 && _selected<_count;
    if(hadSelection) memcpy(selected,_dots[_selected].key,32);
    auto& gps=GPS::instance();
    const auto fix=gps.fixStatus();
    _lastKnown=fix==FixStatus::LAST_KNOWN;
    const double lat=fix==FixStatus::LIVE ? gps.lat() : gps.cachedLat();
    const double lon=fix==FixStatus::LIVE ? gps.lon() : gps.cachedLon();
    _haveOwn=fix!=FixStatus::NO_FIX && radar::validPosition(lat,lon);
    auto& cache=HeardAdvertCache::instance();
    const HeardAdvert* nodes=cache.entries();
    bool anyGeo=false;
    for(int i=0;i<cache.count();++i) if(nodes[i].hasGps && radar::validPosition(nodes[i].gpsLat/1e6,nodes[i].gpsLon/1e6)) anyGeo=true;
    if(_autoMode) { _geo=_haveOwn && anyGeo; _autoMode=false; }
    int order[HEARD_ADVERT_CAP];
    for(int i=0;i<cache.count();++i) order[i]=i;
    const uint32_t now=millis();
    std::sort(order,order+cache.count(),[nodes,now](int a,int b){return uint32_t(now-nodes[a].lastHeardMs)<uint32_t(now-nodes[b].lastHeardMs);});
    _count=0; _rangeKm=1;
    for(int i=0;i<cache.count();++i) {
        const auto& n=nodes[order[i]];
        const double a=n.gpsLat/1e6,b=n.gpsLon/1e6;
        if(_geo && (!_haveOwn || !n.hasGps || !radar::validPosition(a,b))) continue;
        auto& dot=_dots[_count++]; memcpy(dot.key,n.pubKey,32);
        dot.km=_geo ? radar::distanceKm(lat,lon,a,b) : 0;
        dot.bearing=_geo ? radar::bearingDeg(lat,lon,a,b) : radar::stableBearing(n.pubKey);
        _rangeKm=std::max(_rangeKm,dot.km);
    }
    _selected=0;
    for(int i=0;i<_count;++i) {
        auto& d=_dots[i]; const auto* n=findNode(d.key);
        // Linear geographic range; every ring is a quarter of the shown range.
        const double radius=_geo ? _radius*d.km/_rangeKm : _radius*radar::hopRadius(n->hops);
        d.x=_cx+(int16_t)std::lround(radius*std::sin(d.bearing*radar::kPi/180));
        d.y=_cy-(int16_t)std::lround(radius*std::cos(d.bearing*radar::kPi/180));
        if(hadSelection && !memcmp(selected,d.key,32)) _selected=i;
    }
    lv_label_set_text(_modeLabel,_geo ? "Geo / N" : "Hops");
    updateInfo(); lv_obj_invalidate(_plot);
}
void RadarScreen::updateInfo() {
    char text[400];
    char scale[80];
    if(_geo) snprintf(scale,sizeof scale,tf("radar_geo_fmt"),_rangeKm,_lastKnown ? t("radar_cached") : "GPS");
    else snprintf(scale,sizeof scale,"%s",t("radar_hops"));
    const auto* n=_count ? findNode(_dots[_selected].key) : nullptr;
    if(n) {
        char name[40];
        if(n->name[0]) snprintf(name,sizeof name,"%s",truncateUtf8(String(n->name),18).c_str());
        else snprintf(name,sizeof name,"%02x%02x%02x%02x",n->pubKey[0],n->pubKey[1],n->pubKey[2],n->pubKey[3]);
        char place[64];
        if(_geo) snprintf(place,sizeof place,"%.1f km / %.0f deg",_dots[_selected].km,_dots[_selected].bearing);
        else snprintf(place,sizeof place,"%u hops",(unsigned)n->hops);
        snprintf(text,sizeof text,"%d/%d %s\n%s\n%s\n%s\n%lus\n%s",_count,HeardAdvertCache::instance().count(),t("radar_nodes"),scale,name,place,(unsigned long)((millis()-n->lastHeardMs)/1000),t(n->type==1 ? "heard_type_chat" : n->type==2 ? "heard_type_repeater" : n->type==3 ? "heard_type_room" : n->type==4 ? "heard_type_sensor" : "radar_nodes"));
    } else snprintf(text,sizeof text,"0/%d %s\n%s\n\n%s",HeardAdvertCache::instance().count(),t("radar_nodes"),scale,_geo && !_haveOwn ? t("radar_no_fix") : t("radar_empty"));
    lv_label_set_text(_info,text);
    for(auto* b:{_prevBtn,_nextBtn,_detailBtn}) {
        if(_count) lv_obj_clear_state(b,LV_STATE_DISABLED); else lv_obj_add_state(b,LV_STATE_DISABLED);
    }
}
void RadarScreen::tick() {
    if(!_screen || lv_obj_has_flag(_screen,LV_OBJ_FLAG_HIDDEN)) return;
    const uint32_t now=millis();
    if(now-_lastRefresh>=1000) { _lastRefresh=now; refresh(); }
    // One refresh per second, only while visible; no extra radio traffic.
}
void RadarScreen::drawCb(lv_event_t* e) {
    auto* self=(RadarScreen*)lv_event_get_user_data(e);
    auto* dc=lv_event_get_draw_ctx(e); lv_area_t a; lv_obj_get_coords(self->_plot,&a);
    lv_point_t centre={(lv_coord_t)(a.x1+self->_cx),(lv_coord_t)(a.y1+self->_cy)};
    lv_draw_arc_dsc_t arc; lv_draw_arc_dsc_init(&arc); arc.color=theme::TEXT_TIMESTAMP(); arc.width=1;
    for(int r=1;r<=4;++r) lv_draw_arc(dc,&arc,&centre,self->_radius*r/4,0,360);
    lv_draw_label_dsc_t label; lv_draw_label_dsc_init(&label); label.font=FONT_SMALL; label.color=theme::TEXT_SECONDARY();
    lv_area_t north={(lv_coord_t)(centre.x-5),a.y1,(lv_coord_t)(centre.x+12),(lv_coord_t)(a.y1+14)};
    lv_draw_label(dc,&label,&north,self->_geo ? "N" : "",nullptr);
    lv_draw_rect_dsc_t dot; lv_draw_rect_dsc_init(&dot); dot.radius=LV_RADIUS_CIRCLE;
    const uint32_t now=millis();
    for(int i=0;i<self->_count;++i) {
        const auto& p=self->_dots[i]; const auto* n=findNode(p.key); if(!n) continue;
        dot.bg_color=nodeColor(n->type); dot.bg_opa=radar::ageOpacity(now,n->lastHeardMs);
        dot.border_width=i==self->_selected ? 2 : 0; dot.border_color=theme::TEXT_PRIMARY();
        int r=i==self->_selected ? 6 : 4;
        lv_area_t area={(lv_coord_t)(a.x1+p.x-r),(lv_coord_t)(a.y1+p.y-r),(lv_coord_t)(a.x1+p.x+r),(lv_coord_t)(a.y1+p.y+r)};
        lv_draw_rect(dc,&dot,&area);
    }
    dot.bg_color=theme::TEXT_PRIMARY(); dot.bg_opa=LV_OPA_COVER; dot.border_width=0;
    lv_area_t me={(lv_coord_t)(centre.x-3),(lv_coord_t)(centre.y-3),(lv_coord_t)(centre.x+3),(lv_coord_t)(centre.y+3)};
    lv_draw_rect(dc,&dot,&me);
}
void RadarScreen::openSelected() {
    if(!_count || _pending) return;
    memcpy(_pendingKey,_dots[_selected].key,32); _pending=true;
    UIManager::swallowTouchUntilRelease();
    lv_async_call([](void* p){auto* self=(RadarScreen*)p; self->_pending=false;
        if(UIManager::instance().currentScreen()==Screen::RADAR)
            UIManager::instance().showHeardAdvertDetails(self->_pendingKey);
    },this);
}
void RadarScreen::clickCb(lv_event_t* e) {
    auto* self=(RadarScreen*)lv_event_get_user_data(e);
    auto* input=lv_indev_get_act(); if(!input) return;
    lv_point_t point; lv_indev_get_point(input,&point); lv_area_t a; lv_obj_get_coords(self->_plot,&a);
    int best=13*13, chosen=-1;
    for(int i=0;i<self->_count;++i) {
        int dx=point.x-a.x1-self->_dots[i].x,dy=point.y-a.y1-self->_dots[i].y;
        if(dx*dx+dy*dy<best){best=dx*dx+dy*dy;chosen=i;}
    }
    if(chosen>=0) {self->_selected=chosen; self->openSelected();}
}
void RadarScreen::backCb(lv_event_t*) {
    UIManager::swallowTouchUntilRelease();
    lv_async_call([](void*){UIManager::instance().showScreen(Screen::HEARD_ADVERTS);},nullptr);
}
void RadarScreen::modeCb(lv_event_t* e) {
    auto* self=(RadarScreen*)lv_event_get_user_data(e); self->_geo=!self->_geo; self->_autoMode=false; self->refresh();
}
void RadarScreen::selectCb(lv_event_t* e) {
    auto* self=(RadarScreen*)lv_event_get_user_data(e); if(!self->_count) return;
    self->_selected=(self->_selected+self->_count+(lv_event_get_target(e)==self->_prevBtn ? -1 : 1))%self->_count;
    self->updateInfo(); lv_obj_invalidate(self->_plot);
}
void RadarScreen::detailsCb(lv_event_t* e) { ((RadarScreen*)lv_event_get_user_data(e))->openSelected(); }
}
