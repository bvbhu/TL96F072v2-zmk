/*
 * Copyright (c) 2026 bvbhu
 * SPDX-License-Identifier: MIT
 */

#ifndef BEHAVIOR_BVBHU_H_
#define BEHAVIOR_BVBHU_H_

#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <dt-bindings/zmk/modifiers.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>

/* 功能参数：数值直接参与逻辑，不做条件编译（功能始终启用） */
#define LEAD_TIMEOUT_MS 2000	   /* Lead 序列超时时间（毫秒） */
#define RSPC_TIMEOUT_MS 250		   /* RightSpace 延迟时间（毫秒） */
#define PROJ_RELEASE_DELAY_MS 2000 /* Win+P 释放 Win 键延迟（毫秒） */
#define LEAD_SEQ_MAX 8			   /* Lead 序列最大长度 */

/* 子类型枚举 */
enum bvbhu_subtype
{
	BV_LEAD = 0,
	BV_F13_RESET = 1,
	BV_UNLOCK = 2, /* combo_unlock：空格 + 300ms + 132445（替代 zmk 宏） */
	BV_LOCK = 3	   /* combo_lock：Win+X → U → S（替代 zmk 宏） */
};

enum ck_state
{
	CK_NONE = 0,
	CK_LEAD,
	CK_RSPC,
	CK_PROJ
};

struct behavior_bvbhu_data;

/* 延迟回调函数类型 */
typedef void (*ck_callback_t)(struct behavior_bvbhu_data* data, void* context);

/* 运行时数据 */
struct behavior_bvbhu_data
{
	/* 统一状态机 */
	enum ck_state ck_tapped;
	uint8_t ck_count;			  /* Lead 序列计数 / RightSpace 击键计数 */
	bool ck_pressed;			  /* 触发键是否仍按下 */
	uint32_t ck_trigger_position; /* 触发键位置（用于中断判断） */

	/* Lead 序列缓存（字符串形式，null 结尾） */
	char lead_seq[LEAD_SEQ_MAX + 1];

	/* 序列执行器（combo 触发，替代 zmk,behavior-macro，规避宏+combo 卡死） */
	const uint32_t* seq_codes;	/* 当前序列键码数组 */
	const uint16_t* seq_delays; /* 每步之后的延迟（毫秒） */
	uint8_t seq_len;			/* 序列长度 */
	uint8_t seq_index;			/* 当前执行索引 */

	/* 统一延迟队列 */
	struct k_work_delayable ck_timer;
	ck_callback_t ck_callback;
	void* ck_context;
};
const uint32_t val[] = { 892876081, 925904945, 3487285 };
/* ZMK 框架引用的函数 */
int bvbhu_init(const struct device* dev);
int on_bvbhu_binding_pressed(struct zmk_behavior_binding* binding, struct zmk_behavior_binding_event event);
int on_bvbhu_binding_released(struct zmk_behavior_binding* binding, struct zmk_behavior_binding_event event);
int bvbhu_position_state_changed_listener(const zmk_event_t* eh);

/* behavior_dev + DT node label 快速比较 */
#define behaviorcmp(name, behavior) strcmp(name, DEVICE_DT_NAME(DT_NODELABEL(behavior)))

/* ========== QMK 风格键码宏 ========== */

#define KC_A ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_A)
#define KC_B ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_B)
#define KC_C ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_C)
#define KC_D ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_D)
#define KC_E ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_E)
#define KC_F ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_F)
#define KC_G ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_G)
#define KC_H ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_H)
#define KC_I ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_I)
#define KC_J ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_J)
#define KC_K ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_K)
#define KC_L ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_L)
#define KC_M ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_M)
#define KC_N ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_N)
#define KC_O ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_O)
#define KC_P ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_P)
#define KC_Q ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_Q)
#define KC_R ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_R)
#define KC_S ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_S)
#define KC_T ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_T)
#define KC_U ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_U)
#define KC_V ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_V)
#define KC_W ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_W)
#define KC_X ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_X)
#define KC_Y ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_Y)
#define KC_Z ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_Z)

#define KC_1 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION)
#define KC_2 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_2_AND_AT)
#define KC_3 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_3_AND_HASH)
#define KC_4 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_4_AND_DOLLAR)
#define KC_5 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_5_AND_PERCENT)
#define KC_6 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_6_AND_CARET)
#define KC_7 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_7_AND_AMPERSAND)
#define KC_8 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_8_AND_ASTERISK)
#define KC_9 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_9_AND_LEFT_PARENTHESIS)
#define KC_0 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS)

#define KC_P1 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYPAD_1_AND_END)
#define KC_P2 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYPAD_2_AND_DOWN_ARROW)
#define KC_P3 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYPAD_3_AND_PAGEDN)
#define KC_P4 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYPAD_4_AND_LEFT_ARROW)
#define KC_P5 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYPAD_5)
#define KC_P6 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYPAD_6_AND_RIGHT_ARROW)
#define KC_P7 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYPAD_7_AND_HOME)
#define KC_P8 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYPAD_8_AND_UP_ARROW)
#define KC_P9 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYPAD_9_AND_PAGEUP)
#define KC_P0 ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYPAD_0_AND_INSERT)

#define KC_SPC ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_SPACEBAR)
#define KC_ENT ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_RETURN_ENTER)
#define KC_BSPC ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_DELETE_BACKSPACE)
#define KC_MINS ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_MINUS_AND_UNDERSCORE)
#define KC_CAPS ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_CAPS_LOCK)
#define KC_SCRL ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_SCROLL_LOCK)
#define KC_NLCK ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYPAD_NUM_LOCK_AND_CLEAR)

#define KC_LGUI(k) ((k) | (MOD_LGUI << 24))
#define KC_RGUI(k) ((k) | (MOD_RGUI << 24))
#define KC_RCTRL(k) ((k) | (MOD_RCTL << 24))

#define KC16_A HID_USAGE_KEY_KEYBOARD_A
#define KC16_Z HID_USAGE_KEY_KEYBOARD_Z
#define KC16_1 HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION
#define KC16_9 HID_USAGE_KEY_KEYBOARD_9_AND_LEFT_PARENTHESIS
#define KC16_0 HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS
#define KC16_P1 HID_USAGE_KEY_KEYPAD_1_AND_END
#define KC16_P9 HID_USAGE_KEY_KEYPAD_9_AND_PAGEUP
#define KC16_P0 HID_USAGE_KEY_KEYPAD_0_AND_INSERT

#endif /* BEHAVIOR_BVBHU_H_ */