#include "AirViewScreen.h"
#include "UIManager.h"
#include "ModalDialog.h"
#include "theme.h"
#include "../hal/Display.h"
#include "../i18n/I18n.h"
#include "../mesh/MeshManager.h"
#include "../util/AirViewData.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mclite {
namespace {
lv_color_t packetColor(const AirViewData::Event& event) {
    if (event.failed) return lv_color_hex(0xff5555);
    if (event.tx) return lv_color_hex(0xde70ff);
    switch (event.type) {
        case 4: return lv_color_hex(0x56d879); // ADV green
        case 5: return lv_color_hex(0x40d9ff); // CH cyan
        case 2: return lv_color_hex(0xffa94d); // DM orange
        case 3: return lv_color_hex(0xffdf50); // ACK yellow
        default: return theme::TEXT_SECONDARY();
    }
}
}
void AirViewScreen::create(lv_obj_t* parent) {
    _screen = lv_win_create(parent, theme::CHAT_HEADER_HEIGHT);
    lv_obj_set_size(_screen, Display::width(), Display::height() - theme::STATUS_BAR_HEIGHT - theme::FOOTER_HEIGHT);
    lv_obj_align(_screen, LV_ALIGN_BOTTOM_MID, 0, -theme::FOOTER_HEIGHT);
    lv_obj_set_style_bg_color(_screen, theme::BG_PRIMARY(), 0);
    lv_obj_set_style_border_width(_screen, 0, 0);
    lv_obj_set_style_radius(_screen, 0, 0);
    auto* header = lv_win_get_header(_screen);
    lv_obj_set_style_bg_color(header, theme::BG_STATUS_BAR(), 0);
    lv_obj_set_style_pad_all(header, 2, 0);
    auto button = [this, header](const char* text, int width, lv_event_cb_t callback) {
        auto* btn = lv_btn_create(header);
        lv_obj_set_size(btn, width, theme::CHAT_HEADER_HEIGHT - 4);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_set_style_bg_color(btn, theme::BG_INPUT(), 0);
        lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, this);
        auto* label = lv_label_create(btn);
        lv_label_set_text(label, text);
        lv_obj_set_style_text_font(label, FONT_BODY, 0);
        lv_obj_center(label);
        return btn;
    };
    _back = button(LV_SYMBOL_LEFT, theme::BTN_HEADER_BACK_W, backCb);
    auto* title = lv_win_add_title(_screen, "AirView");
    lv_obj_set_style_text_font(title, FONT_HEADING, 0);
    lv_obj_set_style_text_color(title, theme::TEXT_PRIMARY(), 0);
    _reset = button("Reset", 54, resetCb);
    _help = button("?", 26, helpCb);
    auto* content = lv_win_get_content(_screen);
    lv_obj_set_style_bg_color(content, theme::BG_PRIMARY(), 0);
    lv_obj_set_style_pad_all(content, 4, 0);
    lv_obj_set_style_pad_row(content, 3, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    auto label = [content]() {
        auto* obj = lv_label_create(content);
        lv_obj_set_width(obj, LV_PCT(100));
        lv_obj_set_style_text_font(obj, FONT_SMALL, 0);
        lv_obj_set_style_text_color(obj, theme::TEXT_PRIMARY(), 0);
        lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
        return obj;
    };
    _stats = label();
    lv_label_set_text(_stats, "");
    _plot = lv_obj_create(content);
    lv_obj_remove_style_all(_plot);
    lv_obj_set_size(_plot, LV_PCT(100), 60);
    lv_obj_clear_flag(_plot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(_plot, drawCb, LV_EVENT_DRAW_MAIN, this);
    auto* axis = label();
    lv_label_set_text(axis, t("air_axis"));
    _feed = label();
    lv_obj_set_flex_grow(_feed, 1);
    lv_label_set_long_mode(_feed, LV_LABEL_LONG_CLIP);
    lv_obj_add_flag(_screen, LV_OBJ_FLAG_HIDDEN);
}
void AirViewScreen::show() {
    AirViewData::instance().begin();
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_HIDDEN);
    auto* group = lv_group_get_default();
    if (group) {
        for (auto* button : {_back, _reset, _help}) lv_group_add_obj(group, button);
        lv_group_focus_obj(_back);
    }
    refresh();
    _lastRefresh = millis();
}
void AirViewScreen::hide() {
    AirViewData::instance().setActive(false);
    if (!_screen) return;
    for (auto* button : {_back, _reset, _help}) lv_group_remove_obj(button);
    lv_obj_add_flag(_screen, LV_OBJ_FLAG_HIDDEN);
}
void AirViewScreen::tick(bool foreground) {
    if (!_screen || lv_obj_has_flag(_screen, LV_OBJ_FLAG_HIDDEN)) return;
    AirViewData::instance().setActive(foreground);
    if (!foreground) return;
    const uint32_t now = millis();
    if (uint32_t(now - _lastRefresh) >= 250) { _lastRefresh = now; refresh(); }
}
void AirViewScreen::refresh() {
    auto& data = AirViewData::instance();
    char live[32];
    const auto* sample = data.sampleCount() ? &data.sampleAt(data.sampleCount() - 1) : nullptr;
    if (!MeshManager::instance().isRadioReady()) snprintf(live, sizeof live, "%s", t("air_offline"));
    else if (sample && sample->valid && uint32_t(millis() - sample->time) <= 500)
        snprintf(live, sizeof live, "%.1f dBm", sample->rssi);
    else snprintf(live, sizeof live, "--");
    const auto& radio = data.radio;
    char stats[200];
    snprintf(stats, sizeof stats, "%.3f MHz SF%u BW%.1f CR4/%u %ddBm\nLive %s | RX%lu TX%lu !%lu",
             radio.frequency, unsigned(radio.sf), radio.bandwidth, unsigned(radio.cr), int(radio.power), live,
             (unsigned long)data.rxCount, (unsigned long)data.txCount, (unsigned long)data.failCount);
    lv_label_set_text(_stats, stats);
    String feed;
    // Newest first. Fixed rows fit the T-Deck; larger boards also keep a compact view.
    for (size_t i = 0; i < std::min(size_t(4), data.eventCount()); ++i) {
        const auto& event = data.eventAt(data.eventCount() - 1 - i);
        char row[120];
        unsigned long seconds = uint32_t(millis() - event.time) / 1000;
        if (event.tx) snprintf(row, sizeof row, "%s %s %uB %lus\n", event.failed ? "TX!" : "TX",
                               AirViewData::typeName(event.type), unsigned(event.length), seconds);
        else snprintf(row, sizeof row, "RX %s %.1f/%.1f %uB %lus\n", AirViewData::typeName(event.type),
                      event.rssi, event.snr, unsigned(event.length), seconds);
        feed += row;
    }
    lv_label_set_text(_feed, feed.length() ? feed.c_str() : t("air_waiting"));
    lv_obj_invalidate(_plot);
}
void AirViewScreen::drawCb(lv_event_t* event) {
    auto* self = static_cast<AirViewScreen*>(lv_event_get_user_data(event));
    auto* ctx = lv_event_get_draw_ctx(event);
    lv_area_t area; lv_obj_get_coords(self->_plot, &area);
    const int width = lv_area_get_width(&area) - 1, height = lv_area_get_height(&area) - 1;
    auto& data = AirViewData::instance();
    const uint32_t now = millis();
    auto xFor = [now, area, width](uint32_t time) {
        return lv_coord_t(area.x2 - double(uint32_t(now - time)) * width / AirViewData::WINDOW_MS);
    };
    auto yFor = [area, height](float rssi) {
        const float bounded = std::max(-140.0f, std::min(-20.0f, rssi));
        return lv_coord_t(area.y2 - (bounded + 140) * height / 120);
    };
    lv_draw_line_dsc_t line; lv_draw_line_dsc_init(&line);
    line.color = theme::TEXT_TIMESTAMP(); line.opa = LV_OPA_40; line.width = 1;
    for (int i = 0; i <= 3; ++i) {
        lv_point_t left = {area.x1, lv_coord_t(area.y1 + height * i / 3)};
        lv_point_t right = {area.x2, left.y};
        lv_draw_line(ctx, &line, &left, &right);
    }
    line.color = theme::ACCENT(); line.opa = LV_OPA_COVER; line.width = 2;
    for (size_t i = 1; i < data.sampleCount(); ++i) {
        const auto& a = data.sampleAt(i - 1); const auto& b = data.sampleAt(i);
        if (!a.valid || !b.valid || uint32_t(now - a.time) > AirViewData::WINDOW_MS ||
            uint32_t(b.time - a.time) > 250) continue;
        lv_point_t first = {xFor(a.time), yFor(a.rssi)}, last = {xFor(b.time), yFor(b.rssi)};
        lv_draw_line(ctx, &line, &first, &last);
    }
    // Coloured event markers: RX at packet RSSI, TX at the top (no fake RSSI).
    lv_draw_rect_dsc_t mark; lv_draw_rect_dsc_init(&mark);
    mark.bg_opa = LV_OPA_COVER; mark.radius = 1;
    for (size_t i = 0; i < data.eventCount(); ++i) {
        const auto& packet = data.eventAt(i);
        if (uint32_t(now - packet.time) > AirViewData::WINDOW_MS) continue;
        if (!packet.tx && !AirViewData::validRssi(packet.rssi)) continue;
        const auto x = xFor(packet.time);
        const auto y = packet.tx ? lv_coord_t(area.y1 + 3) : yFor(packet.rssi);
        lv_area_t dot = {lv_coord_t(std::max(int(area.x1), int(x) - 2)),
                         lv_coord_t(std::max(int(area.y1), int(y) - 2)),
                         lv_coord_t(std::min(int(area.x2), int(x) + 2)),
                         lv_coord_t(std::min(int(area.y2), int(y) + 2))};
        mark.bg_color = packetColor(packet);
        lv_draw_rect(ctx, &mark, &dot);
    }
}
void AirViewScreen::backCb(lv_event_t*) {
    UIManager::swallowTouchUntilRelease();
    lv_async_call([](void*) { UIManager::instance().showSettings(SettingsSection::Radio); }, nullptr);
}
void AirViewScreen::resetCb(lv_event_t* event) {
    AirViewData::instance().begin();
    static_cast<AirViewScreen*>(lv_event_get_user_data(event))->refresh();
}
void AirViewScreen::helpCb(lv_event_t*) {
    ModalDialog::show("AirView", t("air_help"), {t("btn_ok")}, [](lv_obj_t* dialog, int) {
        ModalDialog::close(dialog);
    });
}
}
