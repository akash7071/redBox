#include "ui.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include <stdio.h>

static const char *TAG = "ui";
static lv_obj_t *ip_label  = NULL;
static lv_obj_t *bat_label = NULL;

void create_ui(void) {
  ESP_LOGI(TAG, "Displaying UI");
  lvgl_port_lock(0);

  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_make(0, 0, 0), LV_PART_MAIN);

  // IP / status label — centered on screen
  ip_label = lv_label_create(scr);
  lv_label_set_text(ip_label, "Connecting WiFi...");
  lv_obj_set_style_text_color(ip_label, lv_color_make(0, 255, 0), LV_PART_MAIN);
  lv_obj_align(ip_label, LV_ALIGN_CENTER, 0, 0);

  // Battery label — bottom center
  bat_label = lv_label_create(scr);
  lv_label_set_text(bat_label, "Bat: ---%");
  lv_obj_set_style_text_color(bat_label, lv_color_make(180, 180, 180), LV_PART_MAIN);
  lv_obj_align(bat_label, LV_ALIGN_BOTTOM_MID, 0, -4);

  lvgl_port_unlock();
}

void ui_update_status(const char *status_str) {
  if (!ip_label) return;
  lvgl_port_lock(0);
  lv_label_set_text(ip_label, status_str);
  lvgl_port_unlock();
}

void ui_update_battery(int percent, float voltage) {
  if (!bat_label) return;
  char buf[32];
  snprintf(buf, sizeof(buf), "Bat: %d%% (%.2fV)", percent, voltage);
  lvgl_port_lock(0);
  lv_label_set_text(bat_label, buf);

  // Color-code by level
  lv_color_t color;
  if (percent > 50)       color = lv_color_make(0, 220, 0);   // green
  else if (percent > 20)  color = lv_color_make(255, 165, 0); // orange
  else                    color = lv_color_make(255, 40, 40);  // red

  lv_obj_set_style_text_color(bat_label, color, LV_PART_MAIN);
  lvgl_port_unlock();
}
