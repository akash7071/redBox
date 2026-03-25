#include "led.h"
#include "esp_log.h"
#include "led_strip.h"

static const char *TAG = "led";

#define LED_GPIO      46
#define LED_COUNT     1

static led_strip_handle_t strip = NULL;

void init_led(void) {
  ESP_LOGI(TAG, "Initializing WS2812 LED on GPIO%d", LED_GPIO);
  led_strip_config_t strip_cfg = {
      .strip_gpio_num    = LED_GPIO,
      .max_leds          = LED_COUNT,
      .led_model         = LED_MODEL_WS2812,
      .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
      .flags = {
          .invert_out = false,
      },
  };
  led_strip_rmt_config_t rmt_cfg = {
      .clk_src       = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = 10000000, // 10 MHz
      .flags = {
          .with_dma = false,
      },
  };
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &strip));
  led_off();
}

void led_set_color(uint8_t r, uint8_t g, uint8_t b) {
  if (!strip) return;
  ESP_ERROR_CHECK(led_strip_set_pixel(strip, 0, r, g, b));
  ESP_ERROR_CHECK(led_strip_refresh(strip));
}

void led_off(void) {
  if (!strip) return;
  ESP_ERROR_CHECK(led_strip_clear(strip));
}
