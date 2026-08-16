# Copyright (c) 2026 bvbhu
# SPDX-License-Identifier: MIT

if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE src/behaviors/behavior_bvbhu.c)

  # ---- QMK rgb_matrix 灯效移植 ----
  # settings_reset 固件无需包含rgb_matrix
  if(NOT ("${SHIELD}" MATCHES "settings_reset"))
    target_sources(app PRIVATE
      src/rgb_matrix/controller/rgb_matrix.c
      src/rgb_matrix/controller/rgb_matrix_settings.c
      src/rgb_matrix/controller/qmk_compat.c
      src/rgb_matrix/controller/lib8tion.c
      src/rgb_matrix/keymap.c
    )

    target_include_directories(app PRIVATE
      src/rgb_matrix
      src/rgb_matrix/controller
      src/rgb_matrix/animations
      src/rgb_matrix/animations/runners
    )
  endif()
endif()

# 自定义 WS2812 GPIO STM32 驱动（Zephyr 4.1.0 内置驱动仅支持 NRF）
if (CONFIG_WS2812_GPIO_STM32)
  target_sources(drivers__led_strip PRIVATE drivers/led_strip/ws2812_gpio_stm32.c)
endif()
