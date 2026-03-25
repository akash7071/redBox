#include "audio.h"
#include "battery.h"
#include "display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware.h"
#include "led.h"
#include "ui.h"
#include "wifi_station.h"

static const char *TAG = "app_main";

#define BATTERY_POLL_INTERVAL_MS 10000

static int led_state = 1;

static void on_loud_sound(void) {
  led_state = !led_state;
  led_state ? led_set_color(0, 64, 0) : led_set_color(64, 0, 0);
  ESP_LOGI(TAG, "Loud sound detected — LED: %d", led_state);
}

void app_main(void) {
  init_hardware(); // Power hold (GPIO10), backlight (GPIO3), button (GPIO42)
  init_display();
  create_ui();
  init_audio();
  init_led();
  led_set_color(0, 64, 0); // Green on boot
  audio_start_mic_monitor(on_loud_sound);
  init_battery();
  wifi_init_sta();

  int last_button_state = 1;
  int bat_tick = 0;

  // Initial battery reading
  ui_update_battery(battery_get_percent(), battery_get_voltage());

  while (1) {
    int button_state = hardware_get_button_state();

    // Beep and toggle RGB LED on button press (falling edge)
    if (last_button_state == 1 && button_state == 0) {
      led_state = !led_state;
      play_beep();
      led_state ? led_set_color(0, 64, 0) : led_set_color(64, 0, 0);
      ESP_LOGI(TAG, "Button pressed — LED: %d", led_state);
    }
    last_button_state = button_state;

    // Poll battery every 10 seconds
    bat_tick += 10;
    if (bat_tick >= BATTERY_POLL_INTERVAL_MS) {
      bat_tick = 0;
      ui_update_battery(battery_get_percent(), battery_get_voltage());
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
