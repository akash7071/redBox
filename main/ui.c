#include "ui.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

static const char *TAG = "ui";
static lv_obj_t *ip_label = NULL;

void create_ui(void) {
  ESP_LOGI(TAG, "Displaying UI");
  lvgl_port_lock(0);

  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_make(0, 0, 0), LV_PART_MAIN);

  ip_label = lv_label_create(scr);
  lv_label_set_text(ip_label, "Connecting WiFi...");
  lv_obj_set_style_text_color(ip_label, lv_color_make(0, 255, 0), LV_PART_MAIN);
  lv_obj_center(ip_label);

  lvgl_port_unlock();
}

void ui_update_status(const char* status_str) {
  if (ip_label) {
    lvgl_port_lock(0);
    lv_label_set_text(ip_label, status_str);
    lvgl_port_unlock();
  }
}
