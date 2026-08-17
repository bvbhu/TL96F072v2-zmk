/*
 * Copyright (c) 2026 bvbhu
 * SPDX-License-Identifier: MIT
 *
 * RGB UG 键码监听器 + Settings 持久化
 * 监听 &rgb_ug 键码，修改灯光设置，并通过 ZMK settings 子系统持久化配置。
 */

#include "lib8tion.h"
#include "rgb_matrix.h"


#include <string.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <dt-bindings/zmk/rgb.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ===== Settings 持久化 ===== */

#define RGB_MATRIX_SETTINGS_KEY "rgb_matrix/state"

static bool settings_loaded = false;
static struct k_work_delayable rgb_matrix_save_work;

static void rgb_matrix_save_handler(struct k_work* work)
{
	(void)work;

	uint64_t raw = rgb_matrix_config.raw;

	int err = settings_save_one(
		RGB_MATRIX_SETTINGS_KEY,
		&raw,
		sizeof(raw)
	);

	if(err)
	{
		LOG_ERR("RGB matrix settings save failed (%d)", err);
	}
}

/* 防抖保存：RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE 定义于 config.h，
 *  与 ZMK 全局 CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE 无关。
 *  - 负值：禁用持久化，不启动调度，不写 Flash
 *  - 0：立即写入
 *  - 正值：尾缘防抖，停止按键后经过此时间才写一次 Flash */
void rgb_matrix_settings_save(void)
{
	if(RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE < 0)
	{
		return;
	}
	k_work_reschedule(&rgb_matrix_save_work, K_MSEC(RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE));
}

static int rgb_matrix_settings_set(const char* name, size_t len, settings_read_cb read_cb, void* cb_arg)
{
	(void)len; /* NVS 后端只传 1 字节哨兵值，不代表真实数据长度 */

	if(strcmp(name, "state") != 0)
	{
		return -ENOENT;
	}

	uint64_t raw;
	ssize_t read_len = read_cb(cb_arg, &raw, sizeof(raw));
	if(read_len != sizeof(raw))
	{
		return -EINVAL;
	}

	rgb_matrix_config.raw = raw;
	settings_loaded = true;
	return 0;
}

static int rgb_matrix_settings_export(int (*cb)(const char* name,
												const void* value,
												size_t val_len))
{
	uint64_t raw = rgb_matrix_config.raw;

	return cb("state", &raw, sizeof(raw));
}

static int rgb_matrix_settings_commit(void)
{
	/* settings_load() 完成后调用，若未加载到有效配置则写入默认值 */
	if(!settings_loaded || !rgb_matrix_config.mode ||
	   rgb_matrix_config.mode >= RGB_MATRIX_EFFECT_MAX)
	{
		eeconfig_update_rgb_matrix_default();
	}
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(rgb_matrix, "rgb_matrix",
							   NULL,						/* h_get */
							   rgb_matrix_settings_set,		/* h_set */
							   rgb_matrix_settings_commit,	/* h_commit */
							   rgb_matrix_settings_export); /* h_export */

/* 初始化 settings 延迟保存工作项 */
void rgb_matrix_settings_init(void)
{
	k_work_init_delayable(&rgb_matrix_save_work, rgb_matrix_save_handler);
}

/* ===== 键码监听器 ===== */

/* behavior_dev + DT node label 快速比较 */
#define behaviorcmp(name, behavior) strcmp(name, DEVICE_DT_NAME(DT_NODELABEL(behavior)))

/* 检查 Shift 是否未按下（Shift 取反用于反向调节） */
#define shift_not_held() (!(zmk_hid_get_explicit_mods() & (MOD_LSFT | MOD_RSFT)))

/* &rgb_ug RGB 键码路由到 RGB Matrix 控制器 */
static int rgb_ug_listener(const zmk_event_t* eh)
{
	const struct zmk_position_state_changed* ev = as_zmk_position_state_changed(eh);
	if(ev == NULL) return ZMK_EV_EVENT_BUBBLE;

	uint8_t layer = zmk_keymap_highest_layer_active();
	const struct zmk_behavior_binding* binding = zmk_keymap_get_layer_binding_at_idx(layer, ev->position);
	if(binding == NULL) return ZMK_EV_EVENT_BUBBLE;

	/* &rgb_ug RGB 键码路由到 RGB Matrix 控制器 */
	if(behaviorcmp(binding->behavior_dev, rgb_ug) == 0 && ev->state)
	{
		switch(binding->param1)
		{
			case RGB_TOG_CMD:
				rgb_matrix_config.enable ^= 1;
				rgb_task_state = STARTING;
				break;
			case RGB_ON_CMD:
				if(!rgb_matrix_config.enable) rgb_task_state = STARTING;
				rgb_matrix_config.enable = 1;
				break;
			case RGB_OFF_CMD:
				if(rgb_matrix_config.enable) rgb_task_state = STARTING;
				rgb_matrix_config.enable = 0;
				break;
			case RGB_HUI_CMD:
				if(rgb_matrix_config.enable)
				{
					if(shift_not_held())
						rgb_matrix_config.hsv.h += RGB_MATRIX_HUE_STEP;
					else
						rgb_matrix_config.hsv.h -= RGB_MATRIX_HUE_STEP;
				}
				break;
			case RGB_HUD_CMD:
				if(rgb_matrix_config.enable)
				{
					if(shift_not_held())
						rgb_matrix_config.hsv.h -= RGB_MATRIX_HUE_STEP;
					else
						rgb_matrix_config.hsv.h += RGB_MATRIX_HUE_STEP;
				}
				break;
			case RGB_SAI_CMD:
				if(rgb_matrix_config.enable)
				{
					if(shift_not_held())
						rgb_matrix_config.hsv.s = qadd8(rgb_matrix_config.hsv.s, RGB_MATRIX_SAT_STEP);
					else
						rgb_matrix_config.hsv.s = qsub8(rgb_matrix_config.hsv.s, RGB_MATRIX_SAT_STEP);
				}
				break;
			case RGB_SAD_CMD:
				if(rgb_matrix_config.enable)
				{
					if(shift_not_held())
						rgb_matrix_config.hsv.s = qsub8(rgb_matrix_config.hsv.s, RGB_MATRIX_SAT_STEP);
					else
						rgb_matrix_config.hsv.s = qadd8(rgb_matrix_config.hsv.s, RGB_MATRIX_SAT_STEP);
				}
				break;
			case RGB_BRI_CMD:
				if(rgb_matrix_config.enable)
				{
					if(shift_not_held())
						rgb_matrix_config.hsv.v = qadd8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP);
					else
						rgb_matrix_config.hsv.v = qsub8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP);
				}
				break;
			case RGB_BRD_CMD:
				if(rgb_matrix_config.enable)
				{
					if(shift_not_held())
						rgb_matrix_config.hsv.v = qsub8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP);
					else
						rgb_matrix_config.hsv.v = qadd8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP);
				}
				break;
			case RGB_SPI_CMD:
				if(rgb_matrix_config.enable)
				{
					if(shift_not_held())
						rgb_matrix_config.speed = qadd8(rgb_matrix_config.speed, RGB_MATRIX_SPD_STEP);
					else
						rgb_matrix_config.speed = qsub8(rgb_matrix_config.speed, RGB_MATRIX_SPD_STEP);
				}
				break;
			case RGB_SPD_CMD:
				if(rgb_matrix_config.enable)
				{
					if(shift_not_held())
						rgb_matrix_config.speed = qsub8(rgb_matrix_config.speed, RGB_MATRIX_SPD_STEP);
					else
						rgb_matrix_config.speed = qadd8(rgb_matrix_config.speed, RGB_MATRIX_SPD_STEP);
				}
				break;
			case RGB_EFF_CMD:
				if(rgb_matrix_config.enable)
				{
					uint8_t mode;
					if(shift_not_held())
					{
						mode = rgb_matrix_config.mode + 1;
						if(mode >= RGB_MATRIX_EFFECT_MAX) mode = 1;
					}
					else
					{
						mode = rgb_matrix_config.mode - 1;
						if(mode < 1) mode = RGB_MATRIX_EFFECT_MAX - 1;
					}
					rgb_matrix_config.mode = mode;
					rgb_task_state = STARTING;
				}
				break;
			case RGB_EFR_CMD:
				if(rgb_matrix_config.enable)
				{
					uint8_t mode;
					if(shift_not_held())
					{
						mode = rgb_matrix_config.mode - 1;
						if(mode < 1) mode = RGB_MATRIX_EFFECT_MAX - 1;
					}
					else
					{
						mode = rgb_matrix_config.mode + 1;
						if(mode >= RGB_MATRIX_EFFECT_MAX) mode = 1;
					}
					rgb_matrix_config.mode = mode;
					rgb_task_state = STARTING;
				}
				break;
			default:
				break;
		}
		rgb_matrix_settings_save();
		return ZMK_EV_EVENT_CAPTURED;
	}

	return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(rgb_ug_behavior, rgb_ug_listener);
ZMK_SUBSCRIPTION(rgb_ug_behavior, zmk_position_state_changed);

/* ===== 按键事件监听器 (反应式/帧缓冲灯效) =====
 * 监听所有按键的按下/释放，转发给 rgb_matrix_handle_position_event。
 * 不捕获事件 (返回 BUBBLE)，仅旁路记录命中点，不影响其他监听器。 */
#if defined(RGB_MATRIX_KEYREACTIVE_ENABLED) || \
	(defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP))
static int rgb_matrix_key_event_listener(const zmk_event_t* eh)
{
	const struct zmk_position_state_changed* ev = as_zmk_position_state_changed(eh);
	if(ev == NULL) return ZMK_EV_EVENT_BUBBLE;
	rgb_matrix_handle_position_event(ev->position, ev->state);
	return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(rgb_matrix_key_event, rgb_matrix_key_event_listener);
ZMK_SUBSCRIPTION(rgb_matrix_key_event, zmk_position_state_changed);
#endif
