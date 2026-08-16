/*
 * Copyright (c) 2026 bvbhu
 * SPDX-License-Identifier: MIT
 *
 *   自定义行为
 *   BV_LEAD(0)       - Lead 键（combo 触发）
 *   BV_F13_RESET(2)  - F13 复位功能（供 tap-dance 单次调用）
 *
 * 事件监听（本文件，单监听器）：
 *   Lead 拦截 / RSPC 占位符拦截 / 中断检测
 *   Win+P 投影 &kp RG(P) → 手动 Win 管理 + P 发送
 *   RGB 标准键码 &rgb_ug在rgb_ug_listener.c中拦截并处理
 *
 * 占位符方案：keymap 中使用 &bvbhu_right_space
 * listener 中按 behavior_dev 识别并拦截为自定义 RightSpace 逻辑
 */

#define DT_DRV_COMPAT zmk_behavior_bvbhu

#include "behavior_bvbhu.h"

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <string.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/hid.h>
#include <zmk/hid_indicators.h>
#include <zmk/keymap.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static void lead_callback(struct behavior_bvbhu_data* data, void* context);
static void rightspace_callback(struct behavior_bvbhu_data* data, void* context);
static void proj_callback(struct behavior_bvbhu_data* data, void* context);
static void seq_callback(struct behavior_bvbhu_data* data, void* context);

static void ck_schedule(struct behavior_bvbhu_data* data, uint32_t ms, ck_callback_t cb, void* ctx);
static void ck_cancel(struct behavior_bvbhu_data* data);
static void ck_trigger(struct behavior_bvbhu_data* data);
static void ck_timer_handler(struct k_work* work);

static uint32_t macro_to_keycode(const char* dev_name);
static char keycode_to_char(uint32_t keycode);
static void send_string(const char* str, int64_t now);
static void tap_keycode(uint32_t usage, int64_t timestamp);

/* Lead 序列 */
static const struct
{
	const char* input;	/* 输入匹配前缀 */
	const char* output; /* 输出字符串 */
} lead_entries[] = {
	{ "158", (const char*)val },
};

/* ===== 序列执行器数据（combo 触发，替代 zmk,behavior-macro） =====
 * combo_unlock/combo_lock 绑定 zmk 宏会在 combo 释放时卡死系统 workqueue
 * （ZMK issue #2356/#3100 已知问题域），改用本自定义行为分步异步执行。
 * seq_delays[i] = tap 完 codes[i] 后到 tap codes[i+1] 的延迟（毫秒）。 */
static const uint32_t seq_unlock_codes[] = { KC_SPC, KC_1, KC_3, KC_2, KC_4, KC_4, KC_5 };
static const uint16_t seq_unlock_delays[] = { 300, 30, 30, 30, 30, 30 };

static const uint32_t seq_lock_codes[] = { KC_LGUI(KC_X), KC_U, KC_S };
static const uint16_t seq_lock_delays[] = { 50, 50 };

static const struct device* bvbhu_dev;
/* ========== 初始化 ========== */
int bvbhu_init(const struct device* dev)
{
	bvbhu_dev = dev;
	struct behavior_bvbhu_data* data = dev->data;
	k_work_init_delayable(&data->ck_timer, ck_timer_handler);
	return 0;
}
static const struct behavior_driver_api bvbhu_driver_api = {
	.binding_pressed = on_bvbhu_binding_pressed,
	.binding_released = on_bvbhu_binding_released,
};
static struct behavior_bvbhu_data bvbhu_data_0;
BEHAVIOR_DT_DEFINE(DT_NODELABEL(bvbhu), bvbhu_init, NULL, &bvbhu_data_0, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &bvbhu_driver_api);

/* ========== 行为绑定处理 ========== */
int on_bvbhu_binding_pressed(struct zmk_behavior_binding* binding, struct zmk_behavior_binding_event event)
{
	const struct device* dev = zmk_behavior_get_binding(binding->behavior_dev);
	if(dev == NULL) return ZMK_BEHAVIOR_OPAQUE;

	struct behavior_bvbhu_data* data = dev->data;
	enum bvbhu_subtype subtype = (enum bvbhu_subtype)(uintptr_t)binding->param1;
	int64_t now = k_uptime_get();

	switch(subtype)
	{
		case BV_LEAD:
			/* 激活 Lead 模式，重置序列计数 */
			data->ck_tapped = CK_LEAD;
			data->ck_count = 0;
			data->lead_seq[0] = '\0';
			data->ck_trigger_position = event.position;
			/* Lead 指示灯已移除 */
			break;
		case BV_F13_RESET:
		{
			/* 条件复位：Caps ON→关, Scroll ON→关, Num OFF→开 */
			zmk_hid_indicators_t leds = zmk_hid_indicators_get_current_profile();
			if(leds & (1 << 1))
				tap_keycode(KC_CAPS, now);
			if(leds & (1 << 2))
				tap_keycode(KC_SCRL, now);
			if(!(leds & (1 << 0)))
				tap_keycode(KC_NLCK, now);
			break;
		}
		case BV_UNLOCK:
			/* 解锁：空格 + 300ms + 132445（替代 macro_unlock，规避宏+combo 卡死） */
			ck_cancel(data);
			data->seq_codes = seq_unlock_codes;
			data->seq_delays = seq_unlock_delays;
			data->seq_len = ARRAY_SIZE(seq_unlock_codes);
			data->seq_index = 0;
			ck_schedule(data, 0, seq_callback, NULL);
			break;
		case BV_LOCK:
			/* 锁定：Win+X → U → S（替代 macro_lock，规避宏+combo 卡死） */
			ck_cancel(data);
			data->seq_codes = seq_lock_codes;
			data->seq_delays = seq_lock_delays;
			data->seq_len = ARRAY_SIZE(seq_lock_codes);
			data->seq_index = 0;
			ck_schedule(data, 0, seq_callback, NULL);
			break;

		default:
			break;
	}
	return ZMK_BEHAVIOR_OPAQUE;
}

int on_bvbhu_binding_released(struct zmk_behavior_binding* binding, struct zmk_behavior_binding_event event)
{
	const struct device* dev = zmk_behavior_get_binding(binding->behavior_dev);
	if(dev == NULL) return ZMK_BEHAVIOR_OPAQUE;

	enum bvbhu_subtype subtype = (enum bvbhu_subtype)(uintptr_t)binding->param1;

	switch(subtype)
	{
		default:
			break;
	}
	return ZMK_BEHAVIOR_OPAQUE;
}

/* ========== 统一事件监听器 ==========
 *
 *  Lead 拦截（ck_tapped==CK_LEAD，所有按键不传递）
 *  中断检测
 *  RSPC 占位符拦截（&bvbhu_right_space → 自定义 RightSpace 逻辑）
 *  捕获 RG(P) 实现 Win+P
 *  RGB 键码已移至 rgb_ug_listener.c
 */
int bvbhu_position_state_changed_listener(const zmk_event_t* eh)
{
	const struct zmk_position_state_changed* ev = as_zmk_position_state_changed(eh);
	if(ev == NULL) return ZMK_EV_EVENT_BUBBLE;
	if(bvbhu_dev == NULL) return ZMK_EV_EVENT_BUBBLE;

	struct behavior_bvbhu_data* data = bvbhu_dev->data;

	uint8_t layer = zmk_keymap_highest_layer_active();
	const struct zmk_behavior_binding* binding =
		zmk_keymap_get_layer_binding_at_idx(layer, ev->position);
	if(binding == NULL) return ZMK_EV_EVENT_BUBBLE;

	/* Lead 拦截：
	 *  - 按下事件一律拦截（CAPTURED）：字符键被消费记录进 lead_seq，
	 *    终止 Lead 的按键（非字符 &kp / 非 kp 行为）同样被拦截、不产生输出；
	 *  - 释放事件一律放行（BUBBLE）：否则 combo 成员键的释放被吞，
	 *    combo 在 ZMK combo 系统中永远 active，导致 pd/pu 后续无抬起事件 */
	if(data->ck_tapped == CK_LEAD)
	{
		if(!ev->state) /* 释放事件放行，交给 combo 系统与 keymap */
			return ZMK_EV_EVENT_BUBBLE;
		bool is_kp = (binding->behavior_dev != NULL && behaviorcmp(binding->behavior_dev, kp) == 0);
		if(is_kp)
		{
			/* &kp 键码传递给 lead_callback 处理 */
			uint32_t keycode = binding->param1;
			lead_callback(data, (void*)(uintptr_t)keycode);
		}
		else
		{
			/*  macro_1~macro_0，映射到对应键码，其他行为终止 Lead*/
			uint32_t kc = macro_to_keycode(binding->behavior_dev);
			if(kc != 0)
				lead_callback(data, (void*)(uintptr_t)kc);
			else
				lead_callback(data, NULL);
		}
		/* 按下事件一律拦截：字符键被记录进序列，终止键不产生输出 */
		return ZMK_EV_EVENT_CAPTURED;
	}
	/* 中断检测 */
	if(data->ck_tapped != CK_NONE && ev->position != data->ck_trigger_position)
	{
		ck_trigger(data);
		data->ck_tapped = CK_NONE;
	}

	/* RSPC 占位符拦截 → 自定义 RightSpace 逻辑 */
	if(behaviorcmp(binding->behavior_dev, bvbhu_right_space) == 0)
	{
		data->ck_pressed = ev->state;
		if(ev->state)
		{
			if(data->ck_tapped == CK_NONE)
			{
				data->ck_tapped = CK_RSPC;
				data->ck_count = 0;
			}
			data->ck_count++;
			data->ck_trigger_position = ev->position;
			ck_schedule(data, RSPC_TIMEOUT_MS, rightspace_callback, NULL);
			zmk_keymap_layer_activate(1, false);
		}
		else
		{
			zmk_keymap_layer_deactivate(1, false);
		}
		return ZMK_EV_EVENT_CAPTURED;
	}

	/* 捕获 RGUI(KC_P) 实现 Win+P 投影功能 */
	if(binding->param1 == KC_RGUI(KC_P))
	{
		int64_t now = k_uptime_get();
		if(ev->state)
		{
			if(!(zmk_hid_get_explicit_mods() & (MOD_LGUI | MOD_RGUI)))
			{
				zmk_hid_register_mods(MOD_RGUI);
			}
			data->ck_tapped = CK_PROJ;
			data->ck_trigger_position = ev->position;
			ck_cancel(data);
		}
		else
		{
			ck_schedule(data, PROJ_RELEASE_DELAY_MS, proj_callback, NULL);
		}
		raise_zmk_keycode_state_changed_from_encoded(KC_P, ev->state, now);
		return ZMK_EV_EVENT_CAPTURED;
	}

	return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(bvbhu_behavior, bvbhu_position_state_changed_listener);
ZMK_SUBSCRIPTION(bvbhu_behavior, zmk_position_state_changed);

/* ========== 序列执行器（combo 触发，替代 zmk,behavior-macro） ==========
 * 由 ck_timer 逐拍驱动：每拍 tap 一个键码，按 seq_delays 延迟下一拍。
 * 全部在系统 workqueue 上异步执行，不阻塞事件线程。 */
static void seq_callback(struct behavior_bvbhu_data* data, void* context)
{
	(void)context;
	if(data->seq_index >= data->seq_len)
	{
		data->seq_index = 0;
		return;
	}
	int64_t now = k_uptime_get();
	tap_keycode(data->seq_codes[data->seq_index], now);
	data->seq_index++;
	if(data->seq_index < data->seq_len)
	{
		/* seq_delays[i] = tap 完 codes[i] 后到 tap codes[i+1] 的延迟 */
		ck_schedule(data, data->seq_delays[data->seq_index - 1], seq_callback, NULL);
	}
}

/* ========== Lead 超时/终止回调 ==========
 *
 * context != NULL: 由按键按下触发，context = (void*)(uintptr_t)keycode
 *   满足条件（keycode_to_char 有效、未超长）则追加字符并重新计时
 *   否则 fallthrough 处理已记录的序列
 * context == NULL: 由超时或中断触发，直接处理已记录的序列
 */
static void lead_callback(struct behavior_bvbhu_data* data, void* context)
{
	/* 取消之前的调度（对应 QMK ck_cancel_deferred） */
	ck_cancel(data);

	if(context != NULL)
	{
		uint32_t keycode = (uint32_t)(uintptr_t)context;
		char c = keycode_to_char(keycode);
		if(c != '\0' && data->ck_count < LEAD_SEQ_MAX)
		{
			data->lead_seq[data->ck_count] = c;
			data->ck_count++;
			data->lead_seq[data->ck_count] = '\0';
			ck_schedule(data, LEAD_TIMEOUT_MS, lead_callback, NULL); /* 超时触发 context 为 NULL */
			return;
		}
	}

	data->ck_tapped = CK_NONE;
	/* Lead 指示灯已移除 */

	if(data->ck_count < 3)
	{
		return;
	}

	int64_t now = k_uptime_get();

	/* 前缀匹配：lead_seq 是 lead_entries[i].input 的前缀则输出完整字符串 */
	for(size_t i = 0; i < ARRAY_SIZE(lead_entries); i++)
	{
		size_t input_len = strlen(lead_entries[i].input);
		if(data->ck_count <= input_len &&
		   strncmp(data->lead_seq, lead_entries[i].input, data->ck_count) == 0)
		{
			send_string(lead_entries[i].output, now);
			return;
		}
	}

	/* 全数字则依次输出 */
	for(uint8_t i = 0; i < data->ck_count; i++)
	{
		if(data->lead_seq[i] < '0' || data->lead_seq[i] > '9')
			return;
	}
	send_string(data->lead_seq, now);
}

/* ========== RightSpace 回调 ==========
 *
 * 按下后延迟 250ms 触发，根据击键次数执行不同动作
 * 触发时未抬起（ck_pressed==true）则不执行动作
 */
static void rightspace_callback(struct behavior_bvbhu_data* data, void* context)
{
	(void)context;
	data->ck_tapped = CK_NONE;
	if(data->ck_pressed)
	{ /* 触发时仍按下则不执行动作 */
		return;
	}
	int64_t now = k_uptime_get();
	uint8_t count = (zmk_keymap_highest_layer_active() == 0) ? data->ck_count : 0;
	switch(count)
	{
		case 4: /* " - " + Ctrl+V */
			tap_keycode(KC_SPC, now);
			tap_keycode(KC_MINS, now);
			tap_keycode(KC_SPC, now);
			zmk_hid_register_mods(MOD_LCTL);
			tap_keycode(KC_V, now);
			zmk_hid_unregister_mods(MOD_LCTL);
		case 3:
			tap_keycode(KC_BSPC, now);
		case 2:
			tap_keycode(KC_ENT, now);
			break;
		case 1:
			tap_keycode(KC_SPC, now);
			break;
		default:
			break;
	}
}

static void proj_callback(struct behavior_bvbhu_data* data, void* context)
{
	(void)context;
	data->ck_tapped = CK_NONE;
	zmk_hid_unregister_mods(zmk_hid_get_explicit_mods() & (MOD_LGUI | MOD_RGUI));
}

/*  统一延迟队列  */
static void ck_schedule(struct behavior_bvbhu_data* data, uint32_t ms, ck_callback_t cb, void* ctx)
{
	data->ck_callback = cb;
	data->ck_context = ctx;
	k_work_reschedule(&data->ck_timer, K_MSEC(ms));
}
static void ck_cancel(struct behavior_bvbhu_data* data)
{ /* 取消延迟队列 */
	if(data->ck_callback == NULL) return;
	k_work_cancel_delayable(&data->ck_timer);
	data->ck_callback = NULL;
	data->ck_context = NULL;
}
static void ck_trigger(struct behavior_bvbhu_data* data)
{ /* 立即触发回调 */
	ck_callback_t cb = data->ck_callback;
	void* ctx = data->ck_context;
	ck_cancel(data);
	if(cb) cb(data, ctx);
}
static void ck_timer_handler(struct k_work* work)
{
	struct k_work_delayable* dwork = CONTAINER_OF(work, struct k_work_delayable, work);
	struct behavior_bvbhu_data* data = CONTAINER_OF(dwork, struct behavior_bvbhu_data, ck_timer);
	ck_trigger(data);
}

/* macro_1~macro_0 → 主键区键码 */
static uint32_t macro_to_keycode(const char* dev_name)
{
	if(dev_name == NULL) return 0;
	if(behaviorcmp(dev_name, macro_1) == 0) return KC_1;
	if(behaviorcmp(dev_name, macro_2) == 0) return KC_2;
	if(behaviorcmp(dev_name, macro_3) == 0) return KC_3;
	if(behaviorcmp(dev_name, macro_4) == 0) return KC_4;
	if(behaviorcmp(dev_name, macro_5) == 0) return KC_5;
	if(behaviorcmp(dev_name, macro_6) == 0) return KC_6;
	if(behaviorcmp(dev_name, macro_7) == 0) return KC_7;
	if(behaviorcmp(dev_name, macro_8) == 0) return KC_8;
	if(behaviorcmp(dev_name, macro_9) == 0) return KC_9;
	if(behaviorcmp(dev_name, macro_0) == 0) return KC_0;
	return 0;
}

/* 键码转字符 */
static char keycode_to_char(uint32_t keycode)
{
	if(SELECT_MODS(keycode) != 0) return '\0'; /* 带修饰键无效 */
	uint16_t id = ZMK_HID_USAGE_ID(keycode);
	/* a-z */
	if(id >= KC16_A && id <= KC16_Z)
		return 'a' + (id - KC16_A);
	/* 1-9（主键盘） */
	if(id >= KC16_1 && id <= KC16_9)
		return '1' + (id - KC16_1);
	/* 1-9（小键盘） */
	if(id >= KC16_P1 && id <= KC16_P9)
		return '1' + (id - KC16_P1);
	/* 0（主键盘/小键盘） */
	if(id == KC16_0 || id == KC16_P0)
		return '0';
	return '\0';
}

/* 发送按键点击 */
static void tap_keycode(uint32_t usage, int64_t timestamp)
{
	raise_zmk_keycode_state_changed_from_encoded(usage, true, timestamp);
	raise_zmk_keycode_state_changed_from_encoded(usage, false, timestamp);
}

/* 发送字符串（主键盘键码，依次 tap）
 * 注意：lead_entries 的 output 可能是混淆编码的 uint32 数组（如 val[]），
 * 无 null 终止保证，必须限制最大读取长度，否则越界读相邻内存
 * 会输出垃圾键（表现为"按下 b"）并可能 HardFault 卡死。
 * val[] = uint32_t[3] = 12 字节；lead_seq 最长 LEAD_SEQ_MAX=8，12 均覆盖。 */
#define LEAD_OUTPUT_MAX 12
static void send_string(const char* str, int64_t now)
{
	for(const char* p = str; *p && (p - str) < LEAD_OUTPUT_MAX; p++)
	{
		uint32_t kc = 0;
		if(*p >= 'a' && *p <= 'z')
		{
			kc = ZMK_HID_USAGE(HID_USAGE_KEY, (KC16_A + (*p - 'a')));
		}
		else if(*p >= 'A' && *p <= 'Z')
		{
			kc = ZMK_HID_USAGE(HID_USAGE_KEY, (KC16_A + (*p - 'A')));
		}
		else if(*p >= '1' && *p <= '9')
		{
			kc = ZMK_HID_USAGE(HID_USAGE_KEY, (KC16_1 + (*p - '1')));
		}
		else if(*p == '0')
		{
			kc = KC_0;
		}
		if(kc)
		{
			tap_keycode(kc, now);
			/* 不在此处 k_sleep：send_string 可能被事件监听器/定时器回调
			 * 同步调用，阻塞会冻结事件线程；HID 报告更新本身足够快 */
		}
	}
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
