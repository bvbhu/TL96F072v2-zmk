/* Copyright 2017 Jason Williams
 * Copyright 2017 Jack Humbert
 * Copyright 2018 Yiancar
 * Copyright (c) 2026 bvbhu
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "rgb_matrix_types.h"
#include "rgb_matrix_settings.h"

#ifndef RGB_MATRIX_TIMEOUT
#	define RGB_MATRIX_TIMEOUT 0
#endif

#ifndef RGB_MATRIX_MAXIMUM_BRIGHTNESS
#	define RGB_MATRIX_MAXIMUM_BRIGHTNESS UINT8_MAX
#endif

#ifndef RGB_MATRIX_HUE_STEP
#	define RGB_MATRIX_HUE_STEP 8
#endif

#ifndef RGB_MATRIX_SAT_STEP
#	define RGB_MATRIX_SAT_STEP 16
#endif

#ifndef RGB_MATRIX_VAL_STEP
#	define RGB_MATRIX_VAL_STEP 16
#endif

#ifndef RGB_MATRIX_SPD_STEP
#	define RGB_MATRIX_SPD_STEP 16
#endif

#ifndef RGB_MATRIX_DEFAULT_ON
#	define RGB_MATRIX_DEFAULT_ON true
#endif

#ifndef RGB_MATRIX_DEFAULT_MODE
#	ifdef ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
#		define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CYCLE_LEFT_RIGHT
#	else
// fallback to solid colors if RGB_MATRIX_CYCLE_LEFT_RIGHT is disabled in userspace
#		define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_COLOR
#	endif
#endif

#ifndef RGB_MATRIX_DEFAULT_HUE
#	define RGB_MATRIX_DEFAULT_HUE 0
#endif

#ifndef RGB_MATRIX_DEFAULT_SAT
#	define RGB_MATRIX_DEFAULT_SAT UINT8_MAX
#endif

#ifndef RGB_MATRIX_DEFAULT_VAL
#	define RGB_MATRIX_DEFAULT_VAL RGB_MATRIX_MAXIMUM_BRIGHTNESS
#endif

#ifndef RGB_MATRIX_DEFAULT_SPD
#	define RGB_MATRIX_DEFAULT_SPD UINT8_MAX / 2
#endif

#ifndef RGB_MATRIX_DEFAULT_FLAGS
#	define RGB_MATRIX_DEFAULT_FLAGS LED_FLAG_ALL
#endif

#ifndef RGB_MATRIX_LED_FLUSH_LIMIT
#	define RGB_MATRIX_LED_FLUSH_LIMIT 16
#endif

#ifndef RGB_MATRIX_LED_PROCESS_LIMIT
#	define RGB_MATRIX_LED_PROCESS_LIMIT RGB_MATRIX_LED_COUNT
#endif

struct rgb_matrix_limits_t
{
	uint8_t led_min_index;
	uint8_t led_max_index;
};

struct rgb_matrix_limits_t rgb_matrix_get_limits(uint8_t iter);

#define RGB_MATRIX_USE_LIMITS_ITER(min, max, iter)                   \
	struct rgb_matrix_limits_t limits = rgb_matrix_get_limits(iter); \
	uint8_t min = limits.led_min_index;                              \
	uint8_t max = limits.led_max_index;                              \
	(void)min;                                                       \
	(void)max;

#define RGB_MATRIX_USE_LIMITS(min, max) RGB_MATRIX_USE_LIMITS_ITER(min, max, params->iter)

#define RGB_MATRIX_INDICATOR_SET_COLOR(i, r, g, b) \
	if(i >= led_min && i < led_max)                \
	{                                              \
		rgb_matrix_set_color(i, r, g, b);          \
	}

	#define RGB_MATRIX_TEST_LED_FLAGS() \
	if(!HAS_ANY_FLAGS(g_led_config.flags[i], params->flags)) continue

enum rgb_matrix_effects
{
	RGB_MATRIX_NONE = 0,
#define RGB_MATRIX_EFFECT(name, ...) RGB_MATRIX_##name,
#include "rgb_matrix_effects.inc"
#undef RGB_MATRIX_EFFECT 
	RGB_MATRIX_EFFECT_MAX
};

void eeconfig_update_rgb_matrix_default(void);

uint8_t rgb_matrix_map_row_column_to_led_kb(uint8_t row, uint8_t column, uint8_t* led_i);
uint8_t rgb_matrix_map_row_column_to_led(uint8_t row, uint8_t column, uint8_t* led_i);

/* ===== 按键事件接入 (反应式/帧缓冲灯效) =====
 * QMK 风格：以物理 (row, col) 触发，更新 last_hit_buffer 与帧缓冲。
 * ZMK 适配：rgb_matrix_handle_position_event 将 keymap position 转为 (row, col) 后转发。
 * 仅在启用反应式或热图灯效时可用。 */
#if defined(RGB_MATRIX_KEYREACTIVE_ENABLED) || \
	(defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP))
void rgb_matrix_handle_key_event(uint8_t row, uint8_t column, bool pressed);
void rgb_matrix_handle_position_event(uint32_t position, bool pressed);
#endif
#if defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP)
void process_rgb_matrix_typing_heatmap(uint8_t row, uint8_t column);
#endif

int rgb_matrix_led_index(int index);

void rgb_matrix_set_color(int index, uint8_t red, uint8_t green, uint8_t blue);
void rgb_matrix_set_color_all(uint8_t red, uint8_t green, uint8_t blue);

void rgb_matrix_task(void);

// This runs after another backlight effect and replaces
// colors already set
void rgb_matrix_indicators(void);
bool rgb_matrix_indicators_kb(void);
bool rgb_matrix_indicators_user(void);

void rgb_matrix_indicators_advanced(effect_params_t* params);
bool rgb_matrix_indicators_advanced_kb(uint8_t led_min, uint8_t led_max);
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max);

void rgb_matrix_init(void);

void rgb_matrix_update_pwm_buffers(void);

static inline bool rgb_matrix_check_finished_leds(uint8_t led_idx)
{
#if defined(RGB_MATRIX_SPLIT)
	if(is_keyboard_left())
	{
		uint8_t k_rgb_matrix_split[2] = RGB_MATRIX_SPLIT;
		return led_idx < k_rgb_matrix_split[0];
	}
	else
		return led_idx < RGB_MATRIX_LED_COUNT;
#else
	return led_idx < RGB_MATRIX_LED_COUNT;
#endif
}

extern rgb_config_t rgb_matrix_config;

extern uint32_t g_rgb_timer;
extern const led_config_t g_led_config;
#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
extern last_hit_t g_last_hit_tracker;
#endif
#ifdef RGB_MATRIX_FRAMEBUFFER_EFFECTS
extern uint8_t g_rgb_frame_buffer[MATRIX_ROWS][MATRIX_COLS];
#endif

extern rgb_task_states rgb_task_state;
rgb_t rgb_matrix_hsv_to_rgb(hsv_t hsv);
int rgb_matrix_controller_init(void);
