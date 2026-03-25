#include "battery.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "battery";

// GPIO2 = ADC1 channel 1 on ESP32-S3
#define BAT_ADC_UNIT    ADC_UNIT_1
#define BAT_ADC_CHANNEL ADC_CHANNEL_1  // GPIO2
#define BAT_ADC_ATTEN   ADC_ATTEN_DB_12

// LiPo voltage thresholds (after x2 scaling from voltage divider)
#define BAT_VOLTAGE_FULL  4.2f
#define BAT_VOLTAGE_EMPTY 3.0f

static adc_oneshot_unit_handle_t adc_handle = NULL;

void init_battery(void) {
  ESP_LOGI(TAG, "Initializing battery ADC on GPIO2");
  adc_oneshot_unit_init_cfg_t unit_cfg = {
      .unit_id = BAT_ADC_UNIT,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

  adc_oneshot_chan_cfg_t chan_cfg = {
      .atten    = BAT_ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc_handle, BAT_ADC_CHANNEL, &chan_cfg));
}

float battery_get_voltage(void) {
  if (!adc_handle) return 0.0f;

  int raw = 0;
  ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, BAT_ADC_CHANNEL, &raw));

  // ADC raw -> millivolts (approximate for 12dB, 3.9V ref on ESP32-S3)
  float mv = (raw / 4095.0f) * 3900.0f;

  // x2 for voltage divider on board
  return (mv / 1000.0f) * 2.0f;
}

int battery_get_percent(void) {
  float v = battery_get_voltage();
  if (v >= BAT_VOLTAGE_FULL)  return 100;
  if (v <= BAT_VOLTAGE_EMPTY) return 0;
  return (int)(((v - BAT_VOLTAGE_EMPTY) /
                (BAT_VOLTAGE_FULL - BAT_VOLTAGE_EMPTY)) * 100.0f);
}
