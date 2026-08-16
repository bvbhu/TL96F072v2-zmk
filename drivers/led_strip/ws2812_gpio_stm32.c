/*
 * Copyright (c) 2026 bvbhu
 *
 * SPDX-License-Identifier: MIT
 *
 * 自定义 WS2812 GPIO 驱动（STM32 专用）
 *
 * Zephyr 4.1.0 的内置 ws2812_gpio.c 仅支持 NRF 系列（使用 NRF 专用 GPIO 寄存器
 * 和 NRF 高频时钟控制）。本驱动使用 STM32 GPIO BSRR 寄存器实现 bit-bang，
 * 适用于 STM32F072 等 Cortex-M0 芯片。
 */

#define DT_DRV_COMPAT bvbhu_ws2812_gpio_stm32

#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_gpio_stm32, CONFIG_LED_STRIP_LOG_LEVEL);

/* STM32F072 @ 48MHz 时序常量（NOP 数量）
 * 1 cycle = 1/48MHz ≈ 20.83ns
 * str 指令约 2 cycles，已计入余量
 */
#define DELAY_T1H 32  /* 700ns ≈ 34 cycles - 2 (str) = 32 */
#define DELAY_T1L 27  /* 600ns ≈ 29 cycles - 2 (str) = 27 */
#define DELAY_T0H 15  /* 350ns ≈ 17 cycles - 2 (str) = 15 */
#define DELAY_T0L 36  /* 800ns ≈ 38 cycles - 2 (str) = 36 */

#define NOPS(i, _) "nop\n"
#define NOP_N_TIMES(n) LISTIFY(n, NOPS, ())

/* STM32 GPIO BSRR 寄存器偏移 */
#define GPIO_BSRR_OFFSET 0x18

/* 内联汇编：设置引脚高/低电平 + NOP 延时
 * Cortex-M0 的 str 指令仅支持 r0-r7，使用 "l" 约束
 */
#define SET_HIGH "str %[set], [%[bsrr]]\n"
#define SET_LOW  "str %[reset], [%[bsrr]]\n"

#define ONE_BIT(bsrr_addr, set_val, reset_val) do {                  \
    __asm volatile(SET_HIGH                                          \
                   NOP_N_TIMES(DELAY_T1H)                            \
                   SET_LOW                                           \
                   NOP_N_TIMES(DELAY_T1L)                            \
                   ::                                                \
                   [bsrr] "l" (bsrr_addr),                           \
                   [set] "l" (set_val),                              \
                   [reset] "l" (reset_val));                         \
} while (false)

#define ZERO_BIT(bsrr_addr, set_val, reset_val) do {                 \
    __asm volatile(SET_HIGH                                          \
                   NOP_N_TIMES(DELAY_T0H)                            \
                   SET_LOW                                           \
                   NOP_N_TIMES(DELAY_T0L)                            \
                   ::                                                \
                   [bsrr] "l" (bsrr_addr),                           \
                   [set] "l" (set_val),                              \
                   [reset] "l" (reset_val));                         \
} while (false)

struct ws2812_gpio_stm32_cfg {
    struct gpio_dt_spec gpio;
    uint8_t num_colors;
    const uint8_t *color_mapping;
    size_t length;
    mem_addr_t port_base;  /* GPIO 端口寄存器基地址 */
    uint32_t reset_delay;  /* 复位延时（微秒） */
};

static int ws2812_gpio_stm32_update_rgb(const struct device *dev,
                                         struct led_rgb *pixels,
                                         size_t num_pixels)
{
    const struct ws2812_gpio_stm32_cfg *config = dev->config;
    /* BSRR 寄存器地址 */
    uint32_t bsrr_addr = config->port_base + GPIO_BSRR_OFFSET;
    /* 设置/复位值 */
    uint32_t pin_set = BIT(config->gpio.pin);
    uint32_t pin_reset = BIT(config->gpio.pin) << 16;
    unsigned int key;
    size_t i;

    key = irq_lock();

    /* 按线上传输格式（如 GRB）逐字节 bit-bang，直接从 pixels 读取，
     * 不修改源缓冲区——避免原位转换破坏 led_strip_pixels 导致
     * raindrops 等非全帧重绘灯效隔帧 R/G 颠倒闪烁。 */
    for (i = 0; i < num_pixels; i++) {
        uint8_t r = pixels[i].r;
        uint8_t g = pixels[i].g;
        uint8_t b = pixels[i].b;
        uint8_t j;

        for (j = 0; j < config->num_colors; j++) {
            uint8_t byte_val;
            switch (config->color_mapping[j]) {
            case LED_COLOR_ID_WHITE:
                byte_val = 0;
                break;
            case LED_COLOR_ID_RED:
                byte_val = r;
                break;
            case LED_COLOR_ID_GREEN:
                byte_val = g;
                break;
            case LED_COLOR_ID_BLUE:
                byte_val = b;
                break;
            default:
                irq_unlock(key);
                return -EINVAL;
            }
            /* MSB 先发 */
            for (int32_t k = 7; k >= 0; k--) {
                if (byte_val & BIT(k)) {
                    ONE_BIT(bsrr_addr, pin_set, pin_reset);
                } else {
                    ZERO_BIT(bsrr_addr, pin_set, pin_reset);
                }
            }
        }
    }

    irq_unlock(key);

    /* 复位延时 */
    k_busy_wait(config->reset_delay);

    return 0;
}

static size_t ws2812_gpio_stm32_length(const struct device *dev)
{
    const struct ws2812_gpio_stm32_cfg *config = dev->config;
    return config->length;
}

static DEVICE_API(led_strip, ws2812_gpio_stm32_api) = {
    .update_rgb = ws2812_gpio_stm32_update_rgb,
    .length = ws2812_gpio_stm32_length,
};

#define WS2812_COLOR_MAPPING(inst)                                     \
    static const uint8_t                                               \
        ws2812_gpio_stm32_##inst##_color_mapping[] =                   \
            DT_INST_PROP(inst, color_mapping)

#define WS2812_GPIO_STM32_DEVICE(inst)                                 \
    static int ws2812_gpio_stm32_##inst##_init(const struct device *dev) \
    {                                                                  \
        const struct ws2812_gpio_stm32_cfg *cfg = dev->config;         \
                                                                       \
        if (!gpio_is_ready_dt(&cfg->gpio)) {                           \
            LOG_ERR("GPIO device not ready");                          \
            return -ENODEV;                                            \
        }                                                              \
                                                                       \
        return gpio_pin_configure_dt(&cfg->gpio, GPIO_OUTPUT);         \
    }                                                                  \
                                                                       \
    WS2812_COLOR_MAPPING(inst);                                        \
                                                                       \
    static const struct ws2812_gpio_stm32_cfg                          \
        ws2812_gpio_stm32_##inst##_cfg = {                            \
            .gpio = GPIO_DT_SPEC_INST_GET(inst, gpios),                \
            .num_colors = DT_INST_PROP_LEN(inst, color_mapping),       \
            .color_mapping = ws2812_gpio_stm32_##inst##_color_mapping, \
            .length = DT_INST_PROP(inst, chain_length),                \
            .port_base = DT_REG_ADDR(                                  \
                DT_INST_PHANDLE_BY_IDX(inst, gpios, 0)),               \
            .reset_delay = DT_INST_PROP_OR(inst, reset_delay, 50),     \
    };                                                                 \
                                                                       \
    DEVICE_DT_INST_DEFINE(inst,                                        \
                        ws2812_gpio_stm32_##inst##_init,               \
                        NULL,                                          \
                        NULL,                                          \
                        &ws2812_gpio_stm32_##inst##_cfg,              \
                        POST_KERNEL,                                   \
                        CONFIG_LED_STRIP_INIT_PRIORITY,                \
                        &ws2812_gpio_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_GPIO_STM32_DEVICE)