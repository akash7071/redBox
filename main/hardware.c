#include "hardware.h"
#include "driver/gpio.h"

#define PIN_NUM_BK_LIGHT 3
#define BUTTON_PIN       42

void init_hardware(void) {
  // Keep device powered on battery (GPIO10 HIGH = stay on)
  gpio_reset_pin(10);
  gpio_set_direction(10, GPIO_MODE_OUTPUT);
  gpio_set_level(10, 1);

  // Backlight
  gpio_reset_pin(PIN_NUM_BK_LIGHT);
  gpio_set_direction(PIN_NUM_BK_LIGHT, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_NUM_BK_LIGHT, 1);

  // Right button
  gpio_reset_pin(BUTTON_PIN);
  gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
}

int hardware_get_button_state(void) {
  return gpio_get_level(BUTTON_PIN);
}

void hardware_set_backlight(int state) {
  gpio_set_level(PIN_NUM_BK_LIGHT, state);
}
