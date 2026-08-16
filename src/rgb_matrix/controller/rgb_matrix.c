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

#include "rgb_matrix.h"
#include "lib8tion.h"

#include <dt-bindings/zmk/rgb.h>
#undef RC
#include <dt-bindings/zmk/matrix_transform.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk/activity.h>
#include <zmk/usb.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* LED strip pixel buffer & device (使用 ZMK led_strip API)
 * 显式零初始化为全黑（熄灭），与 QMK 驱动缓冲区初始状态一致。
 * 该缓冲区跨帧保留数据，raindrops 等灯效依赖此行为。 */
static struct led_rgb led_strip_pixels[RGB_MATRIX_LED_COUNT] = { 0 };
static const struct device* led_strip_dev;

#ifndef RGB_MATRIX_CENTER
const led_point_t k_rgb_matrix_center = { 112, 32 };
#else
const led_point_t k_rgb_matrix_center = RGB_MATRIX_CENTER;
#endif

rgb_t rgb_matrix_hsv_to_rgb(hsv_t hsv)
{
	rgb_t rgb;
	uint16_t h = hsv.h, s = hsv.s, v = hsv.v;
	if(hsv.s == 0)
	{
		rgb.r = v, rgb.g = v, rgb.b = v;
		return rgb;
	}

	uint8_t region = (uint8_t)(h * 6 / 255);
	uint8_t remainder = (uint8_t)((h * 2 - region * 85) * 3);
	uint8_t p = (uint8_t)((v * (255 - s)) >> 8);
	uint8_t q = (uint8_t)((v * (255 - ((s * remainder) >> 8))) >> 8);
	uint8_t t = (uint8_t)((v * (255 - ((s * (255 - remainder)) >> 8))) >> 8);

	switch(region)
	{
		case 6:
		case 0:
			rgb.r = (uint8_t)v, rgb.g = t, rgb.b = p;
			break;
		case 1:
			rgb.r = q, rgb.g = (uint8_t)v, rgb.b = p;
			break;
		case 2:
			rgb.r = p, rgb.g = (uint8_t)v, rgb.b = t;
			break;
		case 3:
			rgb.r = p, rgb.g = q, rgb.b = (uint8_t)v;
			break;
		case 4:
			rgb.r = t, rgb.g = p, rgb.b = (uint8_t)v;
			break;
		default:
			rgb.r = (uint8_t)v, rgb.g = p, rgb.b = q;
			break;
	}
	return rgb;
}
// Generic effect runners
#include "rgb_matrix_runners.inc"

// ------------------------------------------
// -----Begin rgb effect includes macros-----
#define RGB_MATRIX_EFFECT(name)
#define RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#include "rgb_matrix_effects.inc"

#undef RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#undef RGB_MATRIX_EFFECT
// -----End rgb effect includes macros-------
// ------------------------------------------

// globals
rgb_config_t rgb_matrix_config; // TODO: would like to prefix this with g_ for global consistancy, do this in another pr
uint32_t g_rgb_timer;
#ifdef RGB_MATRIX_FRAMEBUFFER_EFFECTS
uint8_t g_rgb_frame_buffer[MATRIX_ROWS][MATRIX_COLS] = { { 0 } };
#endif // RGB_MATRIX_FRAMEBUFFER_EFFECTS
#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
last_hit_t g_last_hit_tracker;
#endif // RGB_MATRIX_KEYREACTIVE_ENABLED

// internals
static bool suspend_state = false;
static uint8_t rgb_last_enable = UINT8_MAX;
static uint8_t rgb_last_effect = UINT8_MAX;
static effect_params_t rgb_effect_params = { 0, LED_FLAG_ALL, false };
rgb_task_states rgb_task_state = SYNCING;

// double buffers
static uint32_t rgb_timer_buffer;
#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
static last_hit_t last_hit_buffer;
#endif // RGB_MATRIX_KEYREACTIVE_ENABLED

// split rgb matrix
#if defined(RGB_MATRIX_SPLIT)
const uint8_t k_rgb_matrix_split[2] = RGB_MATRIX_SPLIT;
#endif

/* ===== 按键事件 position → (row, col) 反向映射 (供反应式灯效使用) =====
 * 映射宏 RGB_MATRIX_POS_TO_RC_LEN / RGB_MATRIX_POS_TO_RC_MAP 由 post_config.h 生成 */
#if defined(RGB_MATRIX_KEYREACTIVE_ENABLED) || \
	(defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP))
static const struct
{
	uint8_t row;
	uint8_t col;
} zmk_rgb_pos_to_rc[RGB_MATRIX_POS_TO_RC_LEN] = RGB_MATRIX_POS_TO_RC_MAP;

static bool rgb_matrix_position_to_rc(uint32_t position, uint8_t* row, uint8_t* col)
{
	if(position >= RGB_MATRIX_POS_TO_RC_LEN) return false;
	*row = zmk_rgb_pos_to_rc[position].row;
	*col = zmk_rgb_pos_to_rc[position].col;
	return true;
}
#endif

void eeconfig_update_rgb_matrix_default(void)
{
	rgb_matrix_config.enable = RGB_MATRIX_DEFAULT_ON;
	rgb_matrix_config.mode = RGB_MATRIX_DEFAULT_MODE;
	rgb_matrix_config.hsv = (hsv_t){ RGB_MATRIX_DEFAULT_HSV };
	rgb_matrix_config.speed = RGB_MATRIX_DEFAULT_SPD;
	rgb_matrix_config.flags = RGB_MATRIX_DEFAULT_FLAGS;
}

__attribute__((weak)) uint8_t rgb_matrix_map_row_column_to_led_kb(uint8_t row, uint8_t column, uint8_t* led_i)
{
	return 0;
}

uint8_t rgb_matrix_map_row_column_to_led(uint8_t row, uint8_t column, uint8_t* led_i)
{
	uint8_t led_count = rgb_matrix_map_row_column_to_led_kb(row, column, led_i);
	uint8_t led_index = g_led_config.matrix_co[row][column];
	if(led_index != NO_LED)
	{
		led_i[led_count] = led_index;
		led_count++;
	}
	return led_count;
}

// QMK 兼容：按键事件入口，更新 last_hit_buffer (反应式) 与帧缓冲 (热图)
#if defined(RGB_MATRIX_KEYREACTIVE_ENABLED) || \
	(defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP))
void rgb_matrix_handle_key_event(uint8_t row, uint8_t column, bool pressed)
{
#	ifndef RGB_MATRIX_SPLIT
	if(!is_keyboard_master()) return;
#	endif
#	ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
	uint8_t led[LED_HITS_TO_REMEMBER];
	uint8_t led_count = 0;
#		if defined(RGB_MATRIX_KEYRELEASES)
	if(!pressed)
#		elif defined(RGB_MATRIX_KEYPRESSES)
	if(pressed)
#		endif // defined(RGB_MATRIX_KEYRELEASES)
	{
		led_count = rgb_matrix_map_row_column_to_led(row, column, led);
	}
	if(last_hit_buffer.count + led_count > LED_HITS_TO_REMEMBER)
	{
		memcpy(&last_hit_buffer.x[0], &last_hit_buffer.x[led_count], LED_HITS_TO_REMEMBER - led_count);
		memcpy(&last_hit_buffer.y[0], &last_hit_buffer.y[led_count], LED_HITS_TO_REMEMBER - led_count);
		memcpy(&last_hit_buffer.tick[0], &last_hit_buffer.tick[led_count], (LED_HITS_TO_REMEMBER - led_count) * 2); // 16 bit
		memcpy(&last_hit_buffer.index[0], &last_hit_buffer.index[led_count], LED_HITS_TO_REMEMBER - led_count);
		last_hit_buffer.count = LED_HITS_TO_REMEMBER - led_count;
	}
	for(uint8_t i = 0; i < led_count; i++)
	{
		uint8_t index = last_hit_buffer.count;
		last_hit_buffer.x[index] = g_led_config.point[led[i]].x;
		last_hit_buffer.y[index] = g_led_config.point[led[i]].y;
		last_hit_buffer.index[index] = led[i];
		last_hit_buffer.tick[index] = 0;
		last_hit_buffer.count++;
	}
#	endif // RGB_MATRIX_KEYREACTIVE_ENABLED
#	if defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP)
#		if defined(RGB_MATRIX_KEYRELEASES)
	if(!pressed)
#		else
	if(pressed)
#		endif // defined(RGB_MATRIX_KEYRELEASES)
	{
		if(rgb_matrix_config.mode == RGB_MATRIX_TYPING_HEATMAP)
		{
			process_rgb_matrix_typing_heatmap(row, column);
		}
	}
#	endif // defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP)
}

// ZMK 适配：将 ZMK 的 position 事件转换为物理 (row, col) 后转发给 QMK 处理函数
void rgb_matrix_handle_position_event(uint32_t position, bool pressed)
{
	uint8_t row, column;
	if(!rgb_matrix_position_to_rc(position, &row, &column)) return;
	rgb_matrix_handle_key_event(row, column, pressed);
}
#endif

void rgb_matrix_update_pwm_buffers(void)
{
	if(led_strip_dev)
		led_strip_update_rgb(led_strip_dev, led_strip_pixels, RGB_MATRIX_LED_COUNT);
}

__attribute__((weak)) int rgb_matrix_led_index(int index)
{
#if defined(RGB_MATRIX_SPLIT)
	if(!is_keyboard_left() && index >= k_rgb_matrix_split[0])
	{
		return index - k_rgb_matrix_split[0];
	}
#endif
	return index;
}

void rgb_matrix_set_color(int index, uint8_t red, uint8_t green, uint8_t blue)
{
	uint8_t i = rgb_matrix_led_index(index);
	led_strip_pixels[i].r = red;
	led_strip_pixels[i].g = green;
	led_strip_pixels[i].b = blue;
}

void rgb_matrix_set_color_all(uint8_t red, uint8_t green, uint8_t blue)
{
#if defined(RGB_MATRIX_SPLIT)
	for(uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++)
		rgb_matrix_set_color(i, red, green, blue);
#else
	for(uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++)
	{
		led_strip_pixels[i].r = red;
		led_strip_pixels[i].g = green;
		led_strip_pixels[i].b = blue;
	}
#endif
}

void rgb_matrix_test(void)
{
	// Mask out bits 4 and 5
	// Increase the factor to make the test animation slower (and reduce to make it faster)
	uint8_t factor = 10;
	switch((g_rgb_timer & (0b11 << factor)) >> factor)
	{
		case 0:
		{
			rgb_matrix_set_color_all(20, 0, 0);
			break;
		}
		case 1:
		{
			rgb_matrix_set_color_all(0, 20, 0);
			break;
		}
		case 2:
		{
			rgb_matrix_set_color_all(0, 0, 20);
			break;
		}
		case 3:
		{
			rgb_matrix_set_color_all(20, 20, 20);
			break;
		}
	}
}

static bool rgb_matrix_none(effect_params_t* params)
{
	if(!params->init)
	{
		return false;
	}

	rgb_matrix_set_color_all(0, 0, 0);
	return false;
}

static void rgb_task_timers(void)
{
#if defined(RGB_MATRIX_KEYREACTIVE_ENABLED)
	uint32_t deltaTime = sync_timer_elapsed32(rgb_timer_buffer);
#endif // defined(RGB_MATRIX_KEYREACTIVE_ENABLED)
	rgb_timer_buffer = sync_timer_read32();

	// Update double buffer last hit timers
#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
	uint8_t count = last_hit_buffer.count;
	for(uint8_t i = 0; i < count; ++i)
	{
		if(UINT16_MAX - deltaTime < last_hit_buffer.tick[i])
		{
			last_hit_buffer.count--;
			continue;
		}
		last_hit_buffer.tick[i] += deltaTime;
	}
#endif // RGB_MATRIX_KEYREACTIVE_ENABLED
}

static void rgb_task_sync(void)
{
	// next task
	if(sync_timer_elapsed32(g_rgb_timer) >= RGB_MATRIX_LED_FLUSH_LIMIT) rgb_task_state = STARTING;
}

static void rgb_task_start(void)
{
	// reset iter
	rgb_effect_params.iter = 0;

	// update double buffers
	g_rgb_timer = rgb_timer_buffer;
#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
	g_last_hit_tracker = last_hit_buffer;
#endif // RGB_MATRIX_KEYREACTIVE_ENABLED

	// next task
	rgb_task_state = RENDERING;
}

static void rgb_task_render(uint8_t effect)
{
	bool rendering = false;
	rgb_effect_params.init = (effect != rgb_last_effect) || (rgb_matrix_config.enable != rgb_last_enable);
	if(rgb_effect_params.flags != rgb_matrix_config.flags)
	{
		rgb_effect_params.flags = rgb_matrix_config.flags;
		rgb_matrix_set_color_all(0, 0, 0);
	}

	// each effect can opt to do calculations
	// and/or request PWM buffer updates.
	switch(effect)
	{
		case RGB_MATRIX_NONE:
			rendering = rgb_matrix_none(&rgb_effect_params);
			break;

// ---------------------------------------------
// -----Begin rgb effect switch case macros-----
#define RGB_MATRIX_EFFECT(name, ...)          \
	case RGB_MATRIX_##name:                   \
		rendering = name(&rgb_effect_params); \
		break;
#include "rgb_matrix_effects.inc"
#undef RGB_MATRIX_EFFECT

			// -----End rgb effect switch case macros-------
			// ---------------------------------------------

		// Factory default magic value
		case UINT8_MAX:
		{
			rgb_matrix_test();
			rgb_task_state = FLUSHING;
		}
			return;
	}

	rgb_effect_params.iter++;

	// next task
	if(!rendering)
	{
		rgb_task_state = FLUSHING;
		if(!rgb_effect_params.init && effect == RGB_MATRIX_NONE)
		{
			// We only need to flush once if we are RGB_MATRIX_NONE
			rgb_task_state = SYNCING;
		}
	}
}

static void rgb_task_flush(uint8_t effect)
{
	// update last trackers after the first full render so we can init over several frames
	rgb_last_effect = effect;
	rgb_last_enable = rgb_matrix_config.enable;

	// update pwm buffers
	rgb_matrix_update_pwm_buffers();

	// next task
	rgb_task_state = SYNCING;
}

void rgb_matrix_task(void)
{
	rgb_task_timers();

	// Ideally we would also stop sending zeros to the LED driver PWM buffers
	// while suspended and just do a software shutdown. This is a cheap hack for now.
	bool rgb_idle_off = false;
	static bool prev_idle_off = false;
#if CONFIG_ZMK_IDLE_TIMEOUT > 0
	/* 有线模式下是否自动关闭 RGB：
	 * 定义 RGB_MATRIX_KEEP_ON_WIRED 后，有线（USB）模式不自动关闭，
	 * 无线（BLE）模式仍按空闲超时关闭；不定义则有线/无线均按空闲超时关闭。 */
#if defined(RGB_MATRIX_KEEP_ON_WIRED)
	bool keep_on_wired = zmk_usb_is_powered();
#else
	bool keep_on_wired = false;
#endif
	if(!keep_on_wired && zmk_activity_get_state() == ZMK_ACTIVITY_IDLE)
	{
		rgb_idle_off = true;
	}
#endif

	/* 空闲状态变化时强制重启状态机，确保灯效正确初始化，
	 * 避免从空闲切换回活跃时因状态机停留在 SYNCING 导致灯效卡死。 */
	if(rgb_idle_off != prev_idle_off)
	{
		rgb_task_state = STARTING;
	}
	prev_idle_off = rgb_idle_off;

	uint8_t effect = rgb_matrix_config.enable && !rgb_idle_off ? rgb_matrix_config.mode : RGB_MATRIX_NONE;

	switch(rgb_task_state)
	{
		case STARTING:
			rgb_task_start();
			break;
		case RENDERING:
			rgb_task_render(effect);
			if(effect)
			{
				if(rgb_task_state == FLUSHING)
				{ // ensure we only draw basic indicators once rendering is finished
					rgb_matrix_indicators();
				}
				rgb_matrix_indicators_advanced(&rgb_effect_params);
			}
			break;
		case FLUSHING:
			rgb_task_flush(effect);
			break;
		case SYNCING:
			rgb_task_sync();
			break;
	}
}

__attribute__((weak)) bool rgb_matrix_indicators_modules(void)
{
	return true;
}

void rgb_matrix_indicators(void)
{
	rgb_matrix_indicators_modules();
	rgb_matrix_indicators_kb();
}

__attribute__((weak)) bool rgb_matrix_indicators_kb(void)
{
	return rgb_matrix_indicators_user();
}

__attribute__((weak)) bool rgb_matrix_indicators_user(void)
{
	return true;
}

struct rgb_matrix_limits_t rgb_matrix_get_limits(uint8_t iter)
{
	struct rgb_matrix_limits_t limits = { 0 };
#if defined(RGB_MATRIX_SPLIT)
	limits.led_min_index = 0;
	limits.led_max_index = RGB_MATRIX_LED_COUNT;
	if(is_keyboard_left() && (limits.led_max_index > k_rgb_matrix_split[0])) limits.led_max_index = k_rgb_matrix_split[0];
	if(!(is_keyboard_left()) && (limits.led_min_index < k_rgb_matrix_split[0])) limits.led_min_index = k_rgb_matrix_split[0];
#else
	limits.led_min_index = 0;
	limits.led_max_index = RGB_MATRIX_LED_COUNT;
#endif
	return limits;
}

void rgb_matrix_indicators_advanced(effect_params_t* params)
{
	/* special handling is needed for "params->iter", since it's already been incremented.
	 * Could move the invocations to rgb_task_render, but then it's missing a few checks
	 * and not sure which would be better. Otherwise, this should be called from
	 * rgb_task_render, right before the iter++ line.
	 */
	RGB_MATRIX_USE_LIMITS_ITER(min, max, params->iter - 1);
	rgb_matrix_indicators_advanced_user(min, max);
}

__attribute__((weak)) bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max)
{
	return true;
}

void rgb_matrix_init(void)
{
	led_strip_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_underglow));
	if(!device_is_ready(led_strip_dev))
	{
		LOG_ERR("LED strip device not ready");
		led_strip_dev = NULL;
	}

#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
	g_last_hit_tracker.count = 0;
	for(uint8_t i = 0; i < LED_HITS_TO_REMEMBER; ++i)
	{
		g_last_hit_tracker.tick[i] = UINT16_MAX;
	}

	last_hit_buffer.count = 0;
	for(uint8_t i = 0; i < LED_HITS_TO_REMEMBER; ++i)
	{
		last_hit_buffer.tick[i] = UINT16_MAX;
	}
#endif // RGB_MATRIX_KEYREACTIVE_ENABLED
}

/* ===== 定时调度：条件选择系统 workqueue 或独立 workqueue ===== */
/* 在 config.h 中定义 RGB_WORKQ_STACK_SIZE > 0 启用独立 workqueue。 */

/* rgb_tick_handler - periodic rgb_matrix_task dispatch */
static void rgb_tick_handler(struct k_work* work)
{
	(void)work;
	rgb_matrix_task();
}

K_WORK_DEFINE(rgb_tick_work, rgb_tick_handler);

#if defined(RGB_WORKQ_STACK_SIZE) && RGB_WORKQ_STACK_SIZE > 0
#	define RGB_WORKQ_PRIORITY (CONFIG_MAIN_THREAD_PRIORITY + 1)
static struct k_work_q rgb_work_q;
K_THREAD_STACK_DEFINE(rgb_work_q_stack, RGB_WORKQ_STACK_SIZE);
#endif

static void rgb_timer_handler(struct k_timer* timer)
{
	(void)timer;
#if defined(RGB_WORKQ_STACK_SIZE) && RGB_WORKQ_STACK_SIZE > 0
	k_work_submit_to_queue(&rgb_work_q, &rgb_tick_work); /* 独立 workqueue */
#else
	k_work_submit(&rgb_tick_work); /* 系统 workqueue（省 RAM） */
#endif
}

K_TIMER_DEFINE(rgb_timer, rgb_timer_handler, NULL);

/* Initialize rgb_matrix + start periodic timer */
int rgb_matrix_controller_init(void)
{
	rgb_matrix_settings_init();
	rgb_matrix_init();

#if defined(RGB_WORKQ_STACK_SIZE) && RGB_WORKQ_STACK_SIZE > 0
	/* k_work_q_start 在 Zephyr 3.1 起重命名为 k_work_queue_start，并新增 cfg 参数 */
	k_work_queue_start(&rgb_work_q, rgb_work_q_stack, K_THREAD_STACK_SIZEOF(rgb_work_q_stack), RGB_WORKQ_PRIORITY, NULL);
#endif

	k_timer_start(&rgb_timer, K_MSEC(RGB_MATRIX_LED_FLUSH_LIMIT), K_MSEC(RGB_MATRIX_LED_FLUSH_LIMIT));

	LOG_INF("RGB matrix controller initialized");
	return 0;
}

SYS_INIT(rgb_matrix_controller_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);