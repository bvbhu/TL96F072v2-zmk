# RGB Matrix 移植参考

将 `src/rgb_matrix` 移植到其他 ZMK 键盘需要以下步骤，AI 生成仅供参考。

## 1. 复制文件

```
项目根目录/
├── src/rgb_matrix/           # 复制整个目录
│   ├── config.h              # 键盘硬件配置（按需调整）
│   ├── keymap.c              # led_config + 指示灯（按实际编写）
│   ├── controller/           # RGB 控制器核心（无需修改）
│   └── animations/           # 灯效文件（无需修改）
├── drivers/
│   └── led_strip/
│       └── ws2812_gpio_stm32.c   # STM32 WS2812 GPIO 驱动（非 STM32 可跳过）
└── CMakeLists.txt            # 添加源文件和头文件路径（见第 2 步）
```

## 2. 创建 zephyr/module.yml

ZMK 用户配置仓库中，`config/` 目录是 Zephyr 模块路径，根目录的 `CMakeLists.txt` **不会被自动处理**。需要创建 `zephyr/module.yml` 将根目录注册为 Zephyr 模块：

```yaml
# zephyr/module.yml
name: zmk-keyboard-<your_keyboard>
build:
  cmake: .
  kconfig: Kconfig            # 可选，仅当根目录有自定义 Kconfig（如自定义驱动符号）时
  settings:
    board_root: .
    dts_root: .
```

> **说明**：`cmake: .` 使 Zephyr 构建系统处理根目录的 `CMakeLists.txt`。`board_root: .` 和 `dts_root: .` 将根目录加入板级/设备树搜索路径。`kconfig` 行可选，仅当根目录有自定义 `Kconfig` 文件时才需要。

## 3. 修改 CMakeLists.txt

在 ZMK 项目的 `CMakeLists.txt` 中添加以下内容：

```cmake
# ---- QMK rgb_matrix 灯效移植 ----
# settings_reset 固件无需包含 rgb_matrix
if(NOT ("${SHIELD}" MATCHES "settings_reset"))
  target_sources(app PRIVATE
    src/rgb_matrix/controller/rgb_matrix.c
    src/rgb_matrix/controller/rgb_matrix_settings.c
    src/rgb_matrix/controller/qmk_compat.c
    src/rgb_matrix/controller/lib8tion.c
    src/rgb_matrix/keymap.c
  )

  target_include_directories(app PRIVATE
    src/rgb_matrix
    src/rgb_matrix/controller
    src/rgb_matrix/animations
    src/rgb_matrix/animations/runners
  )
endif()

# 自定义 WS2812 GPIO STM32 驱动（仅 STM32 + GPIO bit-bang 需要）
if (CONFIG_WS2812_GPIO_STM32)
  target_sources(drivers__led_strip PRIVATE drivers/led_strip/ws2812_gpio_stm32.c)
endif()
```

如果是**分体键盘**，建议包裹条件编译（仅 central 执行渲染）：

```cmake
if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  # 上述 target_sources / target_include_directories 放在此处
endif()
```

## 4. 修改 Board 设备树 (.dts)

**① 添加 chosen 节点**

在 `/chosen` 中添加 `zmk,underglow`：

```dts
chosen {
    zephyr,sram = &sram0;
    zephyr,flash = &flash0;
    zephyr,settings-partition = &storage_partition;  /* 持久化必需 */
    zmk,kscan = &kscan0;
    zmk,matrix_transform = &default_transform;       /* Key Reactive / Framebuffer 灯效必需 */
    zmk,underglow = &led_strip;                     /* RGB Matrix 从该节点获取 LED 设备 */
};
```

**② 添加 LED strip 设备节点**

- **STM32（使用自定义 GPIO 驱动）**：

```dts
led_strip: ws2812 {
    compatible = "bvbhu,ws2812-gpio-stm32";
    chain-length = <96>;            /* LED 总数 */
    gpios = <&gpiob 12 GPIO_ACTIVE_HIGH>;
    color-mapping = <LED_COLOR_ID_GREEN
                     LED_COLOR_ID_RED
                     LED_COLOR_ID_BLUE>;
    reset-delay = <50>;
};
```

> `compatible` 字段串 `bvbhu,ws2812-gpio-stm32` 对应 `drivers/led_strip/ws2812_gpio_stm32.c` 中的 `DT_DRV_COMPAT`。需在 `dts/bindings/` 中提供对应的绑定文件（或参考该驱动自行添加）。
>
> **驱动缓冲区行为**：本驱动的 `update_rgb` 在 bit-bang 时直接按 `color-mapping` 顺序从 `pixels` 读取并发送，**不修改源缓冲区**。这保证了 raindrops、starlight 等非全帧重绘灯效的正确性（它们依赖缓冲区跨帧保留数据）。若使用其他 WS2812 驱动，请确保其 `update_rgb` 同样不修改传入的 `pixels` 缓冲区。

- **NRF（使用 Zephyr 内置 SPI/GPIO 驱动）**：使用 ZMK 原有的 `zmk,underglow` 灯带配置即可，`compatible` 通常为 `worldsemi,ws2812-spi` 或 `worldsemi,ws2812-gpio`。

**③ 添加 flash 分区（持久化必需）**

在 `&flash0` 中添加 `storage_partition`，用于保存 RGB 配置：

```dts
&flash0 {
    partitions {
        compatible = "fixed-partitions";
        #address-cells = <1>;
        #size-cells = <1>;

        storage_partition: partition@1e800 {
            label = "storage";
            reg = <0x0001e800 DT_SIZE_K(6)>;
        };
    };
};
```

分区位置和大小需根据芯片 flash 总大小调整，确保不覆盖固件区域。

## 5. 修改 Board defconfig

在 `boards/<厂商>/<键盘>/<键盘>_defconfig` 中按需添加以下配置：

```kconfig
# 禁用 ZMK 内置 RGB Underglow（由自定义 RGB Matrix 取代）
# 不定义 CONFIG_ZMK_RGB_UNDERGLOW

# （按需）WS2812 GPIO 驱动（STM32 GPIO bit-bang）
CONFIG_WS2812_GPIO_STM32=y

# HID 指示灯支持（Caps/Num/Scroll Lock 检测）
CONFIG_ZMK_HID_INDICATORS=y

# 持久化存储（RGB 设置保存到 flash）
CONFIG_SETTINGS=y
CONFIG_NVS=y
CONFIG_FLASH=y
CONFIG_FLASH_PAGE_LAYOUT=y

# 空闲熄灯（单位 ms，0 表示禁用空闲熄灯）
CONFIG_ZMK_IDLE_TIMEOUT=30000

# （可选）调整栈大小——RGB Matrix 使用工作队列执行渲染
# CONFIG_MAIN_STACK_SIZE=512
# CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=768
```

## 6. 修改 keymap.c

将 `src/rgb_matrix/keymap.c` 中的 `g_led_config` 替换为键盘实际的灯珠布局：

- `matrix_co[MATRIX_ROWS][MATRIX_COLS]` — 矩阵坐标到 LED 索引的映射，`NO_LED(255)` 表示该位置无 LED
- `point[RGB_MATRIX_LED_COUNT]` — 每颗 LED 的物理坐标 `{ x, y }`，范围 0~255
- `flags[RGB_MATRIX_LED_COUNT]` — 每颗 LED 的标签（`LED_FLAG_KEYLIGHT`、`LED_FLAG_UNDERGLOW` 等）

`g_led_config` 可直接从 QMK 的 `led_config_t` 复制粘贴，格式完全兼容。RAM 紧张时可将声明改为 `const led_config_t g_led_config`（存入 flash），并同步修改 `controller/rgb_matrix.h` 中的 `extern` 声明为 `const`。

`rgb_matrix_indicators_advanced_user` 函数可自定义指示灯逻辑（如 CapsLock、NumLock、Shift 等指示）。

## 7. 修改 config.h

在 `src/rgb_matrix/config.h` 中配置键盘硬件参数：

| 宏                               | 说明                | 必填 |
| ------------------------------- | ----------------- | -- |
| `MATRIX_ROWS`                   | 矩阵行数              | 是  |
| `MATRIX_COLS`                   | 矩阵列数              | 是  |
| `RGB_MATRIX_LED_COUNT`          | 灯珠总数              | 是  |
| `RGB_MATRIX_CENTER`             | 灯效中心坐标 `{ x, y }` | 推荐 |
| `RGB_MATRIX_MAXIMUM_BRIGHTNESS` | 最大亮度上限            | 可选 |
| `RGB_MATRIX_DEFAULT_MODE`       | 开机默认灯效模式          | 可选 |
| `RGB_MATRIX_DEFAULT_HSV`        | 开机默认颜色            | 可选 |
| `RGB_MATRIX_DEFAULT_SPD`        | 开机默认速度            | 可选 |
| `RGB_MATRIX_DEFAULT_ON`         | 开机默认启用状态          | 可选 |
| `RGB_MATRIX_DEFAULT_FLAGS`      | 灯效默认应用的 LED 类型    | 可选 |
| `RGB_MATRIX_HUE_STEP`           | 色相步进（键控调整增量）     | 可选 |
| `RGB_MATRIX_SAT_STEP`           | 饱和度步进             | 可选 |
| `RGB_MATRIX_VAL_STEP`           | 明度步进              | 可选 |
| `RGB_MATRIX_SPD_STEP`           | 速度步进              | 可选 |
| `RGB_MATRIX_LED_FLUSH_LIMIT`    | LED 刷新间隔（毫秒）      | 可选 |
| `RGB_WORKQ_STACK_SIZE`          | 独立 workqueue 栈大小   | 可选 |
| `RGB_MATRIX_KEEP_ON_WIRED`      | 有线模式空闲不熄灯        | 可选 |
| `ENABLE_RGB_MATRIX_xxx`         | 启用的灯效（按需定义）       | 可选 |

`RGB_MATRIX_CENTER` 参考坐标计算：`x = 255 * 物理列号 / (总列数 - 1)`，`y = 255 * 物理行号 / (总行数 - 1)`。

`RGB_WORKQ_STACK_SIZE` 定义 `> 0` 时使用独立 workqueue 执行渲染（避免阻塞按键，值即线程栈大小）；不定义或为 `0` 则复用系统 workqueue（省 RAM）。

**分体键盘**：取消注释 `RGB_MATRIX_SPLIT` 和 `RGB_MATRIX_IS_LEFT`。

## 8. 键码绑定

该 RGB Matrix 控制器监听 `&rgb_ug` 键码。在 keymap 中使用标准的 `&rgb_ug RGB_TOG`、`&rgb_ug RGB_EFF` 等键码控制灯光。

`&rgb_ug` 是 ZMK 内置的 RGB Underglow 行为节点，无需额外定义。按下 Shift 同时操作可反向调整（例如 `Shift + RGB_HUI` = 色相减少）。

## 9. 常见问题

**Q: 分体键盘如何配置？**
A: 在 `config.h` 中将源文件/头文件包裹在 `if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)` 条件中（仅 central 端运行渲染），并定义 `RGB_MATRIX_SPLIT { 左半LED数, 右半LED数 }`。

**Q: RAM 不足怎么办？**
A: 减少启用的灯效数量（注释掉 `config.h` 中不必要的 `ENABLE_RGB_MATRIX_xxx`），尤其是 Key Reactive 和 Framebuffer 灯效——它们会常驻分配约 178B BSS（详见下文「RAM 注意事项」一节）。可将 `g_led_config` 声明为 `const` 存入 flash。在 defconfig 中可以缩减 `MAIN_STACK_SIZE` 和 `SYSTEM_WORKQUEUE_STACK_SIZE`，但注意避免栈溢出导致内存损坏（HardFault）。

**Q: Key Reactive / Framebuffer 灯效不生效？**
A: 检查板级 `.dts` 的 chosen 节点是否声明了 `zmk,matrix-transform`。position → (row, col) 映射表由该节点自动生成，缺失时相关灯效会被自动禁用。

## 10. RAM 注意事项

Key Reactive 和 Framebuffer 灯效会常驻分配额外 BSS（与开机默认灯效无关）：

| 灯效类型 | 触发宏                              | 常驻 BSS                                   | 大小 |
| ---- | -------------------------------- | ---------------------------------------- | ------- |
| Key Reactive  | `RGB_MATRIX_KEYREACTIVE_ENABLED` | `g_last_hit_tracker` + `last_hit_buffer` | ~82 B   |
| Framebuffer  | `RGB_MATRIX_FRAMEBUFFER_EFFECTS` | `g_rgb_frame_buffer[6][16]`              | 96 B    |
| 合计   | <br />                           | <br />                                   | ~178 B  |

在 SRAM 紧张的键盘上（如 STM32F072 16KB），启用这些灯效可能使 `.bss` 越界或工作队列栈溢出（HardFault），表现为刷入固件后键盘无法识别。缓解手段：

- 将 `g_led_config` 声明为 `const` 存入 flash（约节省 384 B RAM）
- 调小 `CONFIG_MAIN_STACK_SIZE` / `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` / `CONFIG_ISR_STACK_SIZE`（注意避免栈溢出）
- 不使用独立 workqueue（`RGB_WORKQ_STACK_SIZE`），继续复用系统 workqueue省 RAM
