// LovyanGFX device for the ESP32-3248S035C: ST7796 panel on HSPI, PWM
// backlight, GT911 capacitive touch on its own I2C bus.
//
// LovyanGFX's autodetect knows the 2.8" CYD (2432S028) but not this board,
// so everything is spelled out here. All the knobs live in config.h.
#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "config.h"

class Display : public lgfx::LGFX_Device {
 public:
  Display() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = HSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = TFT_SPI_WRITE_HZ;
      cfg.freq_read = TFT_SPI_READ_HZ;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = PIN_TFT_SCLK;
      cfg.pin_mosi = PIN_TFT_MOSI;
      cfg.pin_miso = PIN_TFT_MISO;
      cfg.pin_dc = PIN_TFT_DC;
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }

    {
      auto cfg = panel_.config();
      cfg.pin_cs = PIN_TFT_CS;
      cfg.pin_rst = PIN_TFT_RST;
      cfg.pin_busy = -1;
      cfg.panel_width = TFT_NATIVE_WIDTH;
      cfg.panel_height = TFT_NATIVE_HEIGHT;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = true;
      cfg.invert = TFT_INVERT;
      cfg.rgb_order = TFT_RGB_ORDER;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;  // the SD slot is on its own pins (VSPI)
      panel_.config(cfg);
    }

    {
      auto cfg = light_.config();
      cfg.pin_bl = PIN_TFT_BL;
      cfg.invert = false;
      cfg.freq = 12000;
      cfg.pwm_channel = BACKLIGHT_PWM_CHANNEL;
      light_.config(cfg);
      panel_.setLight(&light_);
    }

    {
      auto cfg = touch_.config();
      cfg.x_min = 0;
      cfg.x_max = TFT_NATIVE_WIDTH - 1;
      cfg.y_min = 0;
      cfg.y_max = TFT_NATIVE_HEIGHT - 1;
      cfg.pin_int = PIN_TOUCH_INT;
      cfg.pin_rst = PIN_TOUCH_RST;
      cfg.bus_shared = false;
      cfg.offset_rotation = TOUCH_OFFSET_ROTATION;
      cfg.i2c_port = 1;
      cfg.i2c_addr = TOUCH_I2C_ADDR;
      cfg.pin_sda = PIN_TOUCH_SDA;
      cfg.pin_scl = PIN_TOUCH_SCL;
      cfg.freq = TOUCH_I2C_HZ;
      touch_.config(cfg);
      panel_.setTouch(&touch_);
    }

    setPanel(&panel_);
  }

 private:
  lgfx::Panel_ST7796 panel_;
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;
  lgfx::Touch_GT911 touch_;
};
