/*
 * Copyright (c) 2026 bvbhu
 * SPDX-License-Identifier: MIT
 *
 * qmk_compat.c — QMK API 兼容实现
 * 为 keymap.c 等 ZMK 上层代码提供 QMK 风格的函数。
 * 时间相关基于 Zephyr k_uptime_get_32。
 */

#include "qmk_compat.h"

#include <zephyr/kernel.h>

bool is_keyboard_master(void)
{
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
	return IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL);
#else
	return true;
#endif
}

bool is_keyboard_left(void)
{
#if defined(RGB_MATRIX_IS_LEFT)
	return RGB_MATRIX_IS_LEFT;
#elif IS_ENABLED(CONFIG_ZMK_SPLIT)
	/* ZMK 惯例：central = 左半, peripheral = 右半 */
	return IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL);
#else
	return true;
#endif
}

led_t host_keyboard_led_state(void)
{
	uint8_t raw = (uint8_t)zmk_hid_indicators_get_current_profile();
	led_t state = {
		.num_lock = (raw >> 0) & 1,
		.caps_lock = (raw >> 1) & 1,
		.scroll_lock = (raw >> 2) & 1,
	};
	return state;
}

uint8_t get_mods(void)
{
	return (uint8_t)zmk_hid_get_explicit_mods();
}

uint16_t timer_read(void)
{
	return (uint16_t)k_uptime_get_32();
}

uint16_t timer_elapsed(uint16_t last)
{
	return (uint16_t)(k_uptime_get_32() - last);
}

uint16_t timer_read_fast(void)
{
	return (uint16_t)k_uptime_get_32();
}

uint16_t timer_elapsed_fast(uint16_t last)
{
	return (uint16_t)(k_uptime_get_32() - last);
}

uint32_t sync_timer_read32(void)
{
	return k_uptime_get_32();
}

uint32_t sync_timer_elapsed32(uint32_t last)
{
	return k_uptime_get_32() - last;
}
