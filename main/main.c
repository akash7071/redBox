#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_event.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include <stdio.h>

static const char *TAG = "app";

#define WIFI_SSID "TESLA"
#define WIFI_PASS "stevejobs"

static lv_obj_t *ip_label = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    esp_wifi_connect();
    ESP_LOGI(TAG, "Retry connecting to the AP");
    if (ip_label) {
      lvgl_port_lock(0);
      lv_label_set_text(ip_label, "WiFi Connect Failed. Retrying...");
      lvgl_port_unlock();
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
    if (ip_label) {
      char ip_str[64];
      snprintf(ip_str, sizeof(ip_str), "IP: " IPSTR,
               IP2STR(&event->ip_info.ip));
      lvgl_port_lock(0);
      lv_label_set_text(ip_label, ip_str);
      lvgl_port_unlock();
    }
  }
}

static void wifi_init_sta(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL,
      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL,
      &instance_got_ip));

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = WIFI_SSID,
              .password = WIFI_PASS,
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
          },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(TAG, "wifi_init_sta finished.");
}

// Pin definitions
#define PIN_NUM_SCLK 16
#define PIN_NUM_MOSI 17
#define PIN_NUM_MISO -1
#define PIN_NUM_LCD_DC 7
#define PIN_NUM_LCD_RST 18
#define PIN_NUM_LCD_CS 15
#define PIN_NUM_BK_LIGHT 3
#define BUTTON_PIN 42

// Display configuration
#define LCD_HOST SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define LCD_H_RES 128
#define LCD_V_RES 128

static void init_hardware(void) {
  // Configure Backlight
  gpio_reset_pin(PIN_NUM_BK_LIGHT);
  gpio_set_direction(PIN_NUM_BK_LIGHT, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_NUM_BK_LIGHT, 1);

  // Configure Button
  gpio_reset_pin(BUTTON_PIN);
  gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
}

static void init_display(void) {
  ESP_LOGI(TAG, "Initializing SPI bus");
  spi_bus_config_t buscfg = {
      .sclk_io_num = PIN_NUM_SCLK,
      .mosi_io_num = PIN_NUM_MOSI,
      .miso_io_num = PIN_NUM_MISO,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG, "Installing panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
      .dc_gpio_num = PIN_NUM_LCD_DC,
      .cs_gpio_num = PIN_NUM_LCD_CS,
      .pclk_hz = LCD_PIXEL_CLOCK_HZ,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .spi_mode = 0,
      .trans_queue_depth = 10,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST,
                                           &io_config, &io_handle));

  ESP_LOGI(TAG, "Installing ST7789 panel driver");
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = PIN_NUM_LCD_RST,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16,
  };
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

  // Hardware rotation is handled by LVGL software rotation now
  ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));
  ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  ESP_LOGI(TAG, "Initializing LVGL");
  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  lvgl_port_init(&lvgl_cfg);

  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size = LCD_H_RES * 20,
      .double_buffer = true,
      .hres = LCD_H_RES,
      .vres = LCD_V_RES,
      .monochrome = false,
      .flags = {.buff_dma = true, .sw_rotate = true}};
  lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
}

static void create_ui(void) {
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

void app_main(void) {
  init_hardware();
  init_display();
  create_ui();
  wifi_init_sta();

  int led_state = 1;
  int last_button_state = 1;

  while (1) {
    int button_state = gpio_get_level(BUTTON_PIN);

    // Toggle Backlight on button press (falling edge)
    if (last_button_state == 1 && button_state == 0) {
      led_state = !led_state;
      gpio_set_level(PIN_NUM_BK_LIGHT, led_state);
      ESP_LOGI(TAG, "Button pressed! Backlight state changed to %d", led_state);
    }
    last_button_state = button_state;

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
