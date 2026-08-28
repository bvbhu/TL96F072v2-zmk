# TL96F072v2 ZMK Firmware

适用于 **洛塔洛塔洛** 制作的 **TL96F072v2** PCB（STM32F072）的zmk固件。

由于RAM空间限制，比较容易出现卡死等问题，放弃维护。

本仓库主要作为 [zmk-qmk-rgb-matrix](https://github.com/bvbhu/zmk-qmk-rgb-matrix/tree/main) 模块的测试键盘。

键位图推荐使用 [ZMK Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) 查看。

固件使用 [GitHub Actions](https://github.com/bvbhu/TL96F072v2-zmk/actions) 自动构建，使用 QMK Toolbox 刷入。

<img width="480" height="181" alt="键盘图片1" src="https://github.com/user-attachments/assets/43bbfab6-0a9f-4a59-9983-aa52a0bc6fb5" />
<img width="480" height="187" alt="键盘图片2" src="https://github.com/user-attachments/assets/d82f63c4-31b0-4a66-9504-26c5ff799f2e" />

## RGB Matrix

使用[zmk-qmk-rgb-matrix](https://github.com/bvbhu/zmk-qmk-rgb-matrix/tree/main) 模块提供的自定义 RGB 控制器取代 ZMK 内置 `rgb_underglow`，复刻 QMK RGB Matrix，实现了绝大部分多数功能。

| 文件 | 说明 |
|------|------|
| `config/west.yml` | west 清单，从 `bvbhu/zmk-qmk-rgb-matrix` 拉取模块到 `modules/rgb-matrix` |
| `config/tl96.conf` | 灯效参数：LED 数量/行/列、灯效中心、亮度上限、空闲熄灯、启用灯效列表 |
| `config/tl96.keymap.c` | LED 布局（`g_led_config`）及状态指示灯回调（`rgb_matrix_indicators_advanced_user`） |
| `boards/talo/tl96/tl96_defconfig` | 选择自定义驱动 `WS2812_GPIO_STM32`，禁用内置 `RGB_UNDERGLOW` |

### ws8212 bit-bang 驱动

| 文件 | 说明 |
|------|------|
| `drivers/led_strip/ws2812_gpio_stm32.c` | 基于 GPIO BSRR 寄存器的 bit-bang 时序驱动 |
| `dts/bindings/led_strip/bvbhu,ws2812-gpio-stm32.yaml` | 设备树绑定（`chain-length`、`gpios`、`color-mapping`、`reset-delay`） |
| `boards/talo/tl96/tl96.dts` | `led_strip` 节点定义：PB12、96 颗 LED、GRB 色序 |
| `Kconfig` | 声明 `WS2812_GPIO_STM32` 选项 |

### 自定义behavior

| 文件 | 说明 |
|------|------|
| `src/behaviors/behavior_bvbhu.c` | 行为实现（Lead 序列、解锁/锁定、右空格多态、Win+P 延迟） |
| `src/behaviors/behavior_bvbhu.h` | 接口定义、键码宏、参数常量 |
| `dts/bindings/behaviors/zmk,behavior-bvbhu.yaml` | 设备树绑定（`#binding-cells = <1>`，子类型编号） |
| `dts/bindings/vendor-prefixes.txt` | 注册 `bvbhu` 前缀 |
| `config/bvbhu.dtsi` | 行为实例 `&bvbhu` |
| `config/tl96.keymap` | 键位映射，使用 `&bvbhu`、combo、macro 等引用 |
