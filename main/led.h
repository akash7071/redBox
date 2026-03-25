#pragma once

#include <stdint.h>

void init_led(void);
void led_set_color(uint8_t r, uint8_t g, uint8_t b);
void led_off(void);
