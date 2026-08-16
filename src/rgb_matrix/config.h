/*
 * Copyright (c) 2026 bvbhu
 * SPDX-License-Identifier: MIT
 *
 * QMK 风格的 RGB Matrix 配置文件
 * 此文件包含键盘硬件参数、默认颜色/速度设置以及启用的灯效列表。
 */

#pragma once

#include <stdint.h>

/** ===== 矩阵尺寸与 LED 数量 =====
 *  @brief
 *  MATRIX_ROWS 行数
 *  MATRIX_COLS 列数
 *  RGB_MATRIX_LED_COUNT 灯珠总数
 * 应与led_config_t定义一致
 *	  uint8_t matrix_co[MATRIX_ROWS][MATRIX_COLS];
 * 	  led_point_t point[RGB_MATRIX_LED_COUNT];
 * 	  uint8_t flags[RGB_MATRIX_LED_COUNT];
 */
#define MATRIX_ROWS 6
#define MATRIX_COLS 16
#define RGB_MATRIX_LED_COUNT 96

/* ===== 分体键盘配置 ===== */

/** @brief 分体键盘 LED 拆分点
 *  取消注释以启用分体键盘支持。格式 { 左半LED数, 右半LED数 }。
 *  此配置会启用 rgb_matrix_controller.c 中的分体逻辑，包括 LED 索引重映射和渲染范围裁剪。
 *  主控/从属由 CONFIG_ZMK_SPLIT_ROLE_CENTRAL 自动识别。 */
// #define RGB_MATRIX_SPLIT { 48, 48 }

/** @brief 指定本半是否为左侧
 *  未定义默认central=左, peripheral=右。
 *  可选值: 1 = 左半，0 = 右半。 */
// #define RGB_MATRIX_IS_LEFT 1

/* ===== 渲染参数 ===== */

/** @brief LED 刷新间隔阈值（毫秒）
 *  可选值: 1 ~ 100（毫秒） */
#define RGB_MATRIX_LED_FLUSH_LIMIT 33

/** @brief RGB 独立 workqueue 栈大小（字节）
 *  定义 > 0 使用独立 workqueue（避免阻塞按键），值即线程栈大小。
 *  不定义或为 0 则使用系统 workqueue（省 RAM）。 */
// #define RGB_WORKQ_STACK_SIZE 0 

/** @brief LED 坐标系中心点
 *  用于对称灯效（如 Cycle Left-Right）的中心参考坐标。
 *  可选值: { x, y }，x/y 范围 0~255 */
#define RGB_MATRIX_CENTER { 112, 32 }

/* ===== 步进值（键控调整增量） ===== */

/** @brief RGB_HUI / RGB_HUD 键每次增减的色相幅度。
 *  可选值: 1 ~ 255，QMK默认值 8 */
#define RGB_MATRIX_HUE_STEP 8

/** @brief RGB_SAI / RGB_SAD 键每次增减的饱和度幅度。
 *  可选值: 1 ~ 255，QMK默认值 16 */
#define RGB_MATRIX_SAT_STEP 16

/** @brief RGB_BRI / RGB_BRD 键每次增减的明度幅度。
 *  可选值: 1 ~ 255，QMK默认值 16 */
#define RGB_MATRIX_VAL_STEP 16

/** @brief RGB_SPI / RGB_SPD 键每次增减的速度幅度。
 *  可选值: 1 ~ 255，QMK默认值 16 */
#define RGB_MATRIX_SPD_STEP 16

/* ===== 限制与超时 ===== */

/** @brief 最大亮度上限
 *  可选值: 1 ~ 255，UINT8_MAX = 255 */
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 225

/** @brief 默认 LED 标志
 *  控制灯效默认应用于哪些类型的 LED。
 *  可选值（可组合）:
 *    LED_FLAG_ALL       = 0xFF  — 所有 LED
 *    LED_FLAG_KEYLIGHT  = 0x04  — 按键背光
 *    LED_FLAG_UNDERGLOW = 0x02  — 底灯
 *    LED_FLAG_MODIFIER  = 0x01  — 修饰键
 *    LED_FLAG_INDICATOR = 0x08  — 指示灯 */
#define RGB_MATRIX_DEFAULT_FLAGS LED_FLAG_ALL

/** @brief 有线（USB）模式下 RGB 自动关闭控制
 *  定义后：有线模式下不因空闲自动关闭 RGB（插电无需省电），
 *  无线（BLE）模式仍按 CONFIG_ZMK_IDLE_TIMEOUT 空闲自动关闭。
 *  不定义：有线和无线模式都按 CONFIG_ZMK_IDLE_TIMEOUT 自动关闭（默认行为）。 */
// #define RGB_MATRIX_KEEP_ON_WIRED

/* ===== 默认灯效设置 ===== */
/* 这些值在键盘启动且无有效持久化配置时使用 */
/** @brief 默认 HSV 颜色值
 *  HSV 值（均映射至0-255） */
#define RGB_MATRIX_DEFAULT_HSV 170, 255, 200

/** @brief 默认灯效动画速度
 *  可选值: 0-255，值越大动画变化越快。 */
#define RGB_MATRIX_DEFAULT_SPD 127

/** @brief 开机默认启用状态
 *   可选值: true = 开机自动点亮 RGB，false = 需手动开启。 */
#define RGB_MATRIX_DEFAULT_ON true

/** @brief 开机默认灯效模式
 *  从 rgb_matrix_effects.inc 中列出的灯效枚举值中选择。
 *  可选值: RGB_MATRIX_SOLID_COLOR, RGB_MATRIX_CYCLE_LEFT_RIGHT,
 *          RGB_MATRIX_BREATHING 等，参考下方启用灯效，去除ENABLE_前缀 */
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CYCLE_LEFT_RIGHT

/* ===== 启用灯效 ===== */
/* 定义以下宏以启用对应的 RGB 灯效。未定义的灯效不会被编译，可减小固件体积。
 * 反应式灯效和帧缓冲灯效各至少需要100字节RAM，072真不行吧*/
/* ===== 普通灯效 （非反应式、非帧缓冲） ===== */
#define ENABLE_RGB_MATRIX_ALPHAS_MODS
#define ENABLE_RGB_MATRIX_GRADIENT_UP_DOWN
#define ENABLE_RGB_MATRIX_GRADIENT_LEFT_RIGHT
#define ENABLE_RGB_MATRIX_BREATHING
#define ENABLE_RGB_MATRIX_BAND_SAT
#define ENABLE_RGB_MATRIX_BAND_VAL
#define ENABLE_RGB_MATRIX_BAND_PINWHEEL_SAT
#define ENABLE_RGB_MATRIX_BAND_PINWHEEL_VAL
#define ENABLE_RGB_MATRIX_BAND_SPIRAL_SAT
#define ENABLE_RGB_MATRIX_BAND_SPIRAL_VAL
#define ENABLE_RGB_MATRIX_CYCLE_ALL
#define ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
#define ENABLE_RGB_MATRIX_CYCLE_UP_DOWN
#define ENABLE_RGB_MATRIX_RAINBOW_MOVING_CHEVRON
#define ENABLE_RGB_MATRIX_CYCLE_OUT_IN
#define ENABLE_RGB_MATRIX_CYCLE_OUT_IN_DUAL
#define ENABLE_RGB_MATRIX_CYCLE_PINWHEEL
#define ENABLE_RGB_MATRIX_CYCLE_SPIRAL
#define ENABLE_RGB_MATRIX_DUAL_BEACON
#define ENABLE_RGB_MATRIX_RAINBOW_BEACON
#define ENABLE_RGB_MATRIX_RAINBOW_PINWHEELS
#define ENABLE_RGB_MATRIX_FLOWER_BLOOMING
#define ENABLE_RGB_MATRIX_RAINDROPS
#define ENABLE_RGB_MATRIX_JELLYBEAN_RAINDROPS
#define ENABLE_RGB_MATRIX_HUE_BREATHING
#define ENABLE_RGB_MATRIX_HUE_PENDULUM
#define ENABLE_RGB_MATRIX_HUE_WAVE
#define ENABLE_RGB_MATRIX_PIXEL_RAIN
#define ENABLE_RGB_MATRIX_PIXEL_FLOW
#define ENABLE_RGB_MATRIX_PIXEL_FRACTAL
#define ENABLE_RGB_MATRIX_STARLIGHT
#define ENABLE_RGB_MATRIX_STARLIGHT_DUAL_SAT
#define ENABLE_RGB_MATRIX_STARLIGHT_DUAL_HUE
#define ENABLE_RGB_MATRIX_STARLIGHT_SMOOTH
#define ENABLE_RGB_MATRIX_RIVERFLOW

/* 以下灯效需要额外的RAM空间：启用反应式灯效80 bytes 启用帧缓冲灯效96 bytes。
 * 以下灯效需要将 ZMK 的 keymap position 转换为物理 (row, col)。
 * 映射表由设备树 zmk,matrix-transform 的 map 属性自动生成，若无法生成会自动禁用以下灯效。 */

/* ===== 反应式灯效 ===== */
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_WIDE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTIWIDE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_CROSS
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTICROSS
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_NEXUS
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTINEXUS
#define ENABLE_RGB_MATRIX_SPLASH
#define ENABLE_RGB_MATRIX_MULTISPLASH
#define ENABLE_RGB_MATRIX_SOLID_SPLASH
#define ENABLE_RGB_MATRIX_SOLID_MULTISPLASH
/* ===== 帧缓冲灯效 ===== */
#define ENABLE_RGB_MATRIX_TYPING_HEATMAP // 这同时是反应式灯效和帧缓冲灯效
#define ENABLE_RGB_MATRIX_DIGITAL_RAIN
