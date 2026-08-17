# Copyright (c) 2026 bvbhu
# SPDX-License-Identifier: MIT

if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE src/behaviors/behavior_bvbhu.c)
endif()

# 自定义 WS2812 GPIO STM32 驱动（Zephyr 4.1.0 内置驱动仅支持 NRF）
if (CONFIG_WS2812_GPIO_STM32)
  target_sources(drivers__led_strip PRIVATE drivers/led_strip/ws2812_gpio_stm32.c)
endif()

# 注入 TL96 LED 布局 keymap.c（定义 g_led_config）
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/config)
target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/config/tl96.keymap.c)
