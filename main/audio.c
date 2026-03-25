#include "audio.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/i2s_std.h"
#include "es8311.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "audio";

#define I2C_PORT       I2C_NUM_0
#define I2S_PORT       I2S_NUM_0
#define SAMPLE_RATE    16000
#define MCLK_HZ        (SAMPLE_RATE * 256) // 4.096 MHz

#define I2C_SDA_PIN    5
#define I2C_SCL_PIN    4
#define I2S_MCLK_PIN   6
#define I2S_BCLK_PIN   14
#define I2S_LRCLK_PIN  12
#define I2S_DOUT_PIN   11
#define AMP_ENABLE_PIN 9

static i2s_chan_handle_t tx_chan = NULL;
static es8311_handle_t   codec  = NULL;

void init_audio(void) {
  // Speaker amplifier — off until playback
  gpio_reset_pin(AMP_ENABLE_PIN);
  gpio_set_direction(AMP_ENABLE_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(AMP_ENABLE_PIN, 0);

  // I2C for ES8311 codec
  ESP_LOGI(TAG, "Initializing I2C");
  i2c_config_t i2c_cfg = {
      .mode             = I2C_MODE_MASTER,
      .sda_io_num       = I2C_SDA_PIN,
      .scl_io_num       = I2C_SCL_PIN,
      .sda_pullup_en    = GPIO_PULLUP_ENABLE,
      .scl_pullup_en    = GPIO_PULLUP_ENABLE,
      .master.clk_speed = 100000,
  };
  ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &i2c_cfg));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

  // ES8311 codec init
  ESP_LOGI(TAG, "Initializing ES8311 codec");
  codec = es8311_create(I2C_PORT, ES8311_ADDRRES_0);
  es8311_clock_config_t clk_cfg = {
      .mclk_inverted      = false,
      .sclk_inverted      = false,
      .mclk_from_mclk_pin = true,
      .mclk_frequency     = MCLK_HZ,
      .sample_frequency   = SAMPLE_RATE,
  };
  ESP_ERROR_CHECK(es8311_init(codec, &clk_cfg, ES8311_RESOLUTION_16,
                              ES8311_RESOLUTION_16));
  ESP_ERROR_CHECK(es8311_voice_volume_set(codec, 100, NULL));

  // I2S output channel
  ESP_LOGI(TAG, "Initializing I2S");
  i2s_chan_config_t chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

  i2s_std_config_t std_cfg = {
      .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                      I2S_SLOT_MODE_STEREO),
      .gpio_cfg = {
          .mclk = I2S_MCLK_PIN,
          .bclk = I2S_BCLK_PIN,
          .ws   = I2S_LRCLK_PIN,
          .dout = I2S_DOUT_PIN,
          .din  = I2S_GPIO_UNUSED,
      },
  };
  std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;

  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
  ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
  ESP_LOGI(TAG, "Audio init complete");
}

void play_beep(void) {
  if (!tx_chan) return;

  gpio_set_level(AMP_ENABLE_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(10));

  // 200ms of a 1kHz square wave at 16kHz sample rate
  int16_t buf[100]; // 50 stereo pairs per iteration
  int phase = 0;
  size_t written;
  for (int i = 0; i < 64; i++) {
    for (int j = 0; j < 50; j++) {
      phase++;
      int16_t val    = (phase % 16 < 8) ? 30000 : -30000;
      buf[2 * j]     = val;
      buf[2 * j + 1] = val;
    }
    i2s_channel_write(tx_chan, buf, sizeof(buf), &written, portMAX_DELAY);
  }

  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(AMP_ENABLE_PIN, 0);
}
