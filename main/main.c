#include "audio.h"
#include "display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware.h"
#include "led.h"
#include "ui.h"
#include "wifi_station.h"

static const char *TAG = "app_main";

void app_main(void) {
  init_hardware();
  init_display();
  create_ui();
  init_audio();
  init_led();
  led_set_color(0, 64, 0); // Green on boot
  wifi_init_sta();

  int led_state = 1;
  int last_button_state = 1;

  while (1) {
    int button_state = hardware_get_button_state();

    // Toggle backlight and beep on button press (falling edge)
    if (last_button_state == 1 && button_state == 0) {
      led_state = !led_state;
      // hardware_set_backlight(led_state);
      play_beep();
      led_state ? led_set_color(0, 64, 0) : led_set_color(64, 0, 0);
      ESP_LOGI(TAG, "Button pressed — backlight: %d", led_state);
    }
    last_button_state = button_state;

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
