# RGB Matrix 功能说明

使用自定义 RGB 控制器（`controller/`）取代 ZMK 内置 `rgb_underglow`，复刻 QMK RGB Matrix 功能。监听 `&rgb_ug` 键码实现灯光控制，按下 Shift 可反向调整。

支持全部 QMK RGB Matrix 灯效（含反应式与帧缓冲灯效），通过键码 `RGB_EFF` / `RGB_EFR` 循环切换。亮度范围 0–225（0%~88%），以 33ms 间隔刷新。

## 功能介绍

- **灯效体系**：复刻 QMK RGB Matrix，`config.h` 中默认启用全部灯效。
- **渲染调度**：状态机 `STARTING → RENDERING → FLUSHING → SYNCING`，可用独立 workqueue 渲染（避免阻塞按键）。
- **按键事件**：反应式 / 帧缓冲灯效通过事件监听器接入按键，position 映射由设备树自动生成。
- **指示灯**：`rgb_matrix_indicators_advanced_user` 支持 Caps / Num / Scroll / Shift / Ctrl / GUI / Alt / 层指示。
- **持久化**：灯效模式、HSV、速度、开关状态写入 flash。
- **自动熄灯**：空闲自动关闭 LED，可配置有线模式常亮（`RGB_MATRIX_KEEP_ON_WIRED`）。

## 硬件参数

| 参数     | 值                            |
| ------ | ---------------------------- |
| 主控     | STM32F072XB (TL96F072v2)     |
| LED 驱动 | `bvbhu,ws2812-gpio-stm32`    |
| LED 总数 | 96                           |
| 矩阵     | 6 行 × 16 列                   |
| 中心坐标   | {112, 32}                    |
| 最大亮度   | 225                          |
| 刷新间隔   | 33ms                         |
| 分体键盘   | 否                            |

## 配置项

### config.h

| 宏                               | 值                             | 说明                  |
| ------------------------------- | ----------------------------- | ------------------- |
| `MATRIX_ROWS`                   | 6                             | 矩阵行数                |
| `MATRIX_COLS`                   | 16                            | 矩阵列数                |
| `RGB_MATRIX_LED_COUNT`          | 96                            | LED 总数              |
| `RGB_MATRIX_CENTER`             | {112, 32}                     | 灯效中心坐标              |
| `RGB_MATRIX_LED_FLUSH_LIMIT`    | 33                            | 刷新间隔 (ms)           |
| `RGB_MATRIX_HUE_STEP`           | 8                             | 色相步进                |
| `RGB_MATRIX_SAT_STEP`           | 16                            | 饱和度步进               |
| `RGB_MATRIX_VAL_STEP`           | 16                            | 明度步进                |
| `RGB_MATRIX_SPD_STEP`           | 16                            | 速度步进                |
| `RGB_MATRIX_MAXIMUM_BRIGHTNESS` | 225                           | 最大亮度上限              |
| `RGB_MATRIX_DEFAULT_HSV`        | 170, 255, 200                 | 默认颜色                |
| `RGB_MATRIX_DEFAULT_SPD`        | 127                           | 默认速度                |
| `RGB_MATRIX_DEFAULT_ON`         | true                          | 开机默认开启              |
| `RGB_MATRIX_DEFAULT_MODE`       | `RGB_MATRIX_CYCLE_LEFT_RIGHT` | 默认灯效                |
| `RGB_MATRIX_DEFAULT_FLAGS`      | `LED_FLAG_ALL`                | 默认 LED 标志           |
| `RGB_WORKQ_STACK_SIZE`          | 0 (注释)                       | 复用系统 workqueue（省 RAM） |
| `RGB_MATRIX_KEEP_ON_WIRED`      | 未定义                          | 有线/无线均按空闲超时熄灯       |

**启用的灯效（全部开启）：**

- `ALPHAS_MODS`、`GRADIENT_UP_DOWN`、`GRADIENT_LEFT_RIGHT`、`BREATHING`、`BAND_SAT/VAL`、`BAND_PINWHEEL_SAT/VAL`、`BAND_SPIRAL_SAT/VAL`、`CYCLE_ALL`、`CYCLE_LEFT_RIGHT`、`CYCLE_UP_DOWN`、`RAINBOW_MOVING_CHEVRON`、`CYCLE_OUT_IN`、`CYCLE_OUT_IN_DUAL`、`CYCLE_PINWHEEL`、`CYCLE_SPIRAL`、`DUAL_BEACON`、`RAINBOW_BEACON`、`RAINBOW_PINWHEELS`、`FLOWER_BLOOMING`、`RAINDROPS`、`JELLYBEAN_RAINDROPS`、`HUE_BREATHING`、`HUE_PENDULUM`、`HUE_WAVE`、`PIXEL_RAIN`、`PIXEL_FLOW`、`PIXEL_FRACTAL`、`STARLIGHT` 系列、`RIVERFLOW`
- `SOLID_REACTIVE_SIMPLE`、`SOLID_REACTIVE`、`SOLID_REACTIVE_WIDE`、`SOLID_REACTIVE_MULTIWIDE`、`SOLID_REACTIVE_CROSS`、`SOLID_REACTIVE_MULTICROSS`、`SOLID_REACTIVE_NEXUS`、`SOLID_REACTIVE_MULTINEXUS`、`SPLASH`、`MULTISPLASH`、`SOLID_SPLASH`、`SOLID_MULTISPLASH`
- `TYPING_HEATMAP`、`DIGITAL_RAIN`

### keymap.c

- `g_led_config` — LED 灯珠布局，共 96 颗 LED，格式与 QMK 完全兼容，声明为 `const` 存入 flash
- `rgb_matrix_indicators_advanced_user` — 已实现 Caps / Num / Scroll / Shift / Ctrl / GUI / Alt / 层指示灯

> 反应式/帧缓冲灯效的 position → (row, col) 映射表由设备树 `zmk,matrix-transform` 自动生成，无需手动维护。

### 板级配置

`boards/talo/tl96/tl96_defconfig` 关键配置：

| 配置                                       | 状态     | 说明                 |
| ---------------------------------------- | ------ | ------------------ |
| `CONFIG_ZMK_RGB_UNDERGLOW`               | n (注释) | 由自定义 RGB Matrix 替代 |
| `CONFIG_WS2812_GPIO_STM32`               | y      | 自定义 WS2812 GPIO 驱动 |
| `CONFIG_SETTINGS` / `CONFIG_NVS`          | y      | 持久化存储              |
| `CONFIG_FLASH` / `CONFIG_FLASH_PAGE_LAYOUT` | y      | Flash 支持           |
| `CONFIG_ZMK_HID_INDICATORS`              | y      | Caps/Num/Scroll 检测 |
| `CONFIG_ZMK_IDLE_TIMEOUT`                | 0      | 禁用空闲熄灯             |
| `CONFIG_ZMK_EXT_POWER`                   | n      | 无外部电源控制            |
| `CONFIG_MAIN_STACK_SIZE` / `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` | 640 | 缩减栈省 RAM          |
| `CONFIG_ISR_STACK_SIZE`                  | 1024   | 缩小 ISR 栈           |

`tl96.dts` chosen 节点声明 `zephyr,settings-partition = &storage_partition;`、`zmk,matrix_transform = &default_transform;`、`zmk,underglow = &led_strip;`，并定义 16KB `storage_partition` 用于持久化。

## 文件结构

```
src/rgb_matrix/
├── config.h              # 硬件参数与灯效配置
├── keymap.c              # LED 灯珠布局
├── controller/           # RGB 控制器核心（无需修改）
│   ├── rgb_matrix.c / .h
│   ├── rgb_matrix_settings.c / .h
│   ├── rgb_matrix_types.h
│   ├── qmk_compat.c / .h
│   ├── post_config.h
│   └── lib8tion.c / .h
└── animations/           # 灯效算法（无需修改）
    ├── runners/          # 灯效运行器
    └── *.h               # 各灯效实现
```
