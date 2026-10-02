// ESP-IDF entry point. Starts BTstack and Bluepad32, which then launch the
// Arduino setup()/loop() task (see main.cpp).
//
// Adapted from esp-idf-arduino-bluepad32-template, main/main.c
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Ricardo Quesada

#include "sdkconfig.h"

#include <stddef.h>

#include <btstack_port_esp32.h>
#include <btstack_run_loop.h>
#include <btstack_stdio_esp32.h>

#include <arduino_platform.h>
#include <uni.h>

int app_main(void) {
#ifndef CONFIG_ESP_CONSOLE_UART_NONE
#ifndef CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
  btstack_stdio_init();
#endif
#endif

  // BTstack on the ESP32 VHCI controller
  btstack_init();

  // Must come before uni_init(). The Arduino platform is what spawns the
  // Arduino task and hands controller data to it.
  uni_platform_set_custom(get_arduino_platform());

  uni_init(0, NULL);

  // Never returns: this task is now the Bluetooth run loop.
  btstack_run_loop_execute();
  return 0;
}
