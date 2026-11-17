#include "ModalDialog.h"
#include "theme.h"
#include "UIManager.h"
#include "../hal/Display.h"
#include "../hal/IInput.h"

namespace mclite {

namespace {
struct DlgCtx {
    ModalDialog::Callback cb;
    lv_group_t* group = nullptr;
    lv_obj_t*   scrim = nullptr;
    lv_obj_t*   body  = nullptr;  // scrollable body label (nullptr if none)
};

void dlgBtnCb(lv_event_t* e) {
    lv_obj_t* panel = (lv_obj_t*)lv_event_get_user_data(e);
    if (!panel) return;
    DlgCtx* ctx = (DlgCtx*)lv_obj_get_user_data(panel);
    if (!ctx) return;  // already closing — ignore late taps
    int idx = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    if (ctx->cb) ctx->cb(panel, idx);
}
}  // namespace

lv_obj_t* ModalDialog::show(const String& title, const String& body,
                            const std::vector<String>& buttons, Callback cb,
                            const String& smallTail) {
    DlgCtx* ctx = new DlgCtx();
    ctx->cb = cb;

    // Dim backdrop.
    ctx->scrim = lv_obj_create(lv_layer_top());
    lv_obj_set_size(ctx->scrim, Display::width(), Display::height());
    lv_obj_set_pos(ctx->scrim, 0, 0);
    lv_obj_set_style_bg_color(ctx->scrim, theme::SCRIM(), 0);
    lv_obj_set_style_bg_opa(ctx->scrim, LV_OPA_50, 0);
    lv_obj_set_style_border_width(ctx->scrim, 0, 0);
    lv_obj_set_style_radius(ctx->scrim, 0, 0);
    lv_obj_clear_flag(ctx->scrim, LV_OBJ_FLAG_SCROLLABLE);

    // Card.
    lv_obj_t* panel = lv_obj_create(lv_layer_top());
    lv_obj_set_width(panel, theme::MODAL_TEXT_WIDTH);
    lv_obj_set_height(panel, LV_SIZE_CONTENT);
    lv_obj_set_style_max_height(panel, Display::height() - 2 * theme::PAD_MEDIUM, 0);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(panel, theme::BG_SECONDARY(), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel, theme::ACCENT(), 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 8, 0);
    lv_obj_set_style_pad_all(panel, theme::PAD_MEDIUM, 0);
    lv_obj_set_style_pad_row(panel, theme::PAD_SMALL, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(panel, ctx);

    lv_obj_t* bodyContainer = nullptr;

    if (title.length()) {
        lv_obj_t* lbl = lv_label_create(panel);
        lv_obj_set_width(lbl, LV_PCT(100));
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(lbl, FONT_HEADING, 0);
        lv_obj_set_style_text_color(lbl, theme::TEXT_PRIMARY(), 0);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(lbl, title.c_str());
    }

    if (body.length() || smallTail.length()) {
        // Scrollable container so long bodies (telemetry/advert detail) fit.
        lv_obj_t* bc = lv_obj_create(panel);
        bodyContainer = bc;
        lv_obj_set_width(bc, LV_PCT(100));
        lv_obj_set_height(bc, LV_SIZE_CONTENT);
        lv_obj_set_style_max_height(bc, 150, 0);
        lv_obj_set_style_bg_opa(bc, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(bc, 0, 0);
        lv_obj_set_style_pad_all(bc, 0, 0);
        lv_obj_set_style_pad_row(bc, theme::PAD_SMALL, 0);
        lv_obj_set_flex_flow(bc, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scroll_dir(bc, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(bc, LV_SCROLLBAR_MODE_AUTO);
        if (body.length()) {
            ctx->body = lv_label_create(bc);
            lv_obj_set_width(ctx->body, LV_PCT(100));
            lv_label_set_long_mode(ctx->body, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_font(ctx->body, FONT_NORMAL, 0);
            lv_obj_set_style_text_color(ctx->body, theme::TEXT_PRIMARY(), 0);
            lv_label_set_text(ctx->body, body.c_str());
        }
        if (smallTail.length()) {
            // Smaller, secondary-color tail (raw public keys) so they stay compact.
            lv_obj_t* tail = lv_label_create(bc);
            lv_obj_set_width(tail, LV_PCT(100));
            lv_label_set_long_mode(tail, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_font(tail, FONT_BODY, 0);
            lv_obj_set_style_text_color(tail, theme::TEXT_SECONDARY(), 0);
            lv_obj_set_style_text_line_space(tail, 2, 0);
            lv_label_set_text(tail, smallTail.c_str());
        }
    }

    ctx->group = lv_group_create();
    for (size_t i = 0; i < buttons.size(); i++) {
        lv_obj_t* b = lv_btn_create(panel);
        lv_obj_set_width(b, LV_PCT(100));
        lv_obj_set_height(b, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(b, theme::BG_INPUT(), 0);
        lv_obj_set_style_bg_color(b, theme::ACCENT(), LV_STATE_FOCUSED);
        lv_obj_set_user_data(b, (void*)(intptr_t)i);
        lv_obj_add_event_cb(b, dlgBtnCb, LV_EVENT_CLICKED, panel);
        lv_obj_t* lbl = lv_label_create(b);
        lv_obj_set_style_text_font(lbl, FONT_HEADING, 0);
        lv_obj_set_style_text_color(lbl, theme::TEXT_PRIMARY(), 0);
        lv_label_set_text(lbl, buttons[i].c_str());
        lv_obj_center(lbl);
        lv_group_add_obj(ctx->group, b);
    }

    // Reserve the measured title/button heights before limiting the scrollable
    // body. A fixed 150 px body plus two buttons exceeds the T-Deck's card.
    if (bodyContainer) {
        lv_obj_update_layout(panel);
        const int limit = Display::height() - 2 * theme::PAD_MEDIUM;
        const int border = lv_obj_get_style_border_width(panel,0);
        int reserved = lv_obj_get_style_pad_top(panel,0)
                     + lv_obj_get_style_pad_bottom(panel,0) + 2 * border;
        const uint32_t children = lv_obj_get_child_cnt(panel);
        if (children > 1) reserved += (children - 1) * lv_obj_get_style_pad_row(panel,0);
        for (uint32_t i = 0; i < children; ++i) {
            auto* child = lv_obj_get_child(panel,i);
            if (child != bodyContainer) reserved += lv_obj_get_height(child);
        }
        int bodyHeight = limit - reserved;
        if (bodyHeight > 150) bodyHeight = 150;
        if (bodyHeight < 24) {
            bodyHeight = 24;
            // Unusually many buttons cannot all fit at once; let the card
            // scroll instead of permanently clipping the final controls.
            lv_obj_add_flag(panel,LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_scroll_dir(panel,LV_DIR_VER);
            lv_obj_set_scrollbar_mode(panel,LV_SCROLLBAR_MODE_AUTO);
            lv_obj_set_flex_align(panel,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
        }
        lv_obj_set_style_max_height(bodyContainer,bodyHeight,0);
        lv_obj_update_layout(panel);
    }

    UIManager::instance().switchToModalGroup(panel);
    IInput::instance().attachToGroup(ctx->group);
    return panel;
}

void ModalDialog::setBody(lv_obj_t* dlg, const String& body) {
    if (!dlg) return;
    DlgCtx* ctx = (DlgCtx*)lv_obj_get_user_data(dlg);
    if (ctx && ctx->body) lv_label_set_text(ctx->body, body.c_str());
}

void ModalDialog::close(lv_obj_t* dlg) {
    if (!dlg) return;
    DlgCtx* ctx = (DlgCtx*)lv_obj_get_user_data(dlg);
    lv_obj_set_user_data(dlg, nullptr);  // ignore any late button taps during teardown
    UIManager::instance().restoreFromModalGroup();
    if (ctx && ctx->group) lv_group_del(ctx->group);
    if (ctx && ctx->scrim) lv_obj_del_async(ctx->scrim);
    lv_obj_del_async(dlg);
    // Defer ctx delete: close() is usually called from inside ctx->cb, so the
    // std::function is still on the stack — freeing it now would be use-after-free.
    if (ctx) lv_async_call([](void* p) { delete (DlgCtx*)p; }, ctx);
}

}  // namespace mclite
