# TL96F072v2 ZMK 固件 & QMK RGB Matrix

本项目全部使用 AI 编写。

适用于 **洛塔洛塔洛** 制作的 **TL96F072v2** PCB 的 ZMK 固件，复刻了 QMK RGB Matrix，编写了一些自定义行为。

<img width="480" height="181" alt="键盘图片1" src="https://github.com/user-attachments/assets/43bbfab6-0a9f-4a59-9983-aa52a0bc6fb5" />
<img width="480" height="187" alt="键盘图片2" src="https://github.com/user-attachments/assets/d82f63c4-31b0-4a66-9504-26c5ff799f2e" />

---

### RGB Matrix

使用自定义 RGB 控制器取代 ZMK 内置 `rgb_underglow`，复刻 QMK RGB Matrix。支持全部灯效，灯效/HSV/速度/开关状态自动保存至 flash，空闲可自动熄灯。RGB Matrix 实现由 `zmk-qmk-rgb-matrix` 模块提供，键盘仓仅需通过 Kconfig 和 `keymap.c` 配置参数与 LED 布局。移植参考请参见 [RGB Matrix移植参考.md](<RGB Matrix移植参考.md>)。

---

键位图推荐使用 [ZMK Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) 查看。

---

## 自定义行为

使用 `zmk,behavior-bvbhu` 自定义行为驱动，配合统一事件监听器实现以下功能：

- **`&bvbhu 0` — Lead 序列匹配**：由 Combo（PGDN+PGUP+PSCR）触发，激活后拦截所有按键构建序列，支持前缀匹配，全数字序列自动依次输出；按下非字母数字键即终止 Lead（该键被拦截、不产生输出），按下首个字符键后 2s 无输入超时终止。预设序列见 `src/behaviors/behavior_bvbhu.c` 的 `lead_entries`。
- **`&bvbhu 2` — 解锁序列**：由 Combo（KP_N0+KP_DOT）触发，依次输出 空格 → 300ms → `132445`。
- **`&bvbhu 3` — 锁定序列**：由 Combo（KP_N1+KP_N2+KP_N3）触发，依次输出 `Win+X → U → S`。
- **`&bvbhu_right_space` — 右空格**：按住临时激活层 1（Lower），释放后恢复；单击空格、双击回车、三击退格回车、四击 `"- "` + Ctrl+V。
- **`&f13` — 状态复位 / Alt+F4**：tap-dance 行为，单击切回 0 层并执行 `&bvbhu 1`（关 CapsLock/ScrollLock、开 NumLock），双击发送 Alt+F4。
- **`&kp RG(P)` — 优化 Win+P 投影**：延迟 2s 释放 Win 键。

### Combo 功能

| Combo | 键位 | 行为 |
|-------|------|------|
| 解锁 | KP_N0 + KP_DOT（85+86） | `&bvbhu 2`：空格 + 300ms + `132445` |
| 锁定 | KP_N1 + KP_N2 + KP_N3（70+71+72） | `&bvbhu 3`：`Win+X` → `U` → `S` |
| Lead | PGDN + PGUP + PSCR（22+23+24） | `&bvbhu 0`：激活 Lead 序列模式 |

> 解锁/锁定 Combo 使用自定义行为而非 `zmk,behavior-macro`：ZMK 的 combo→宏 组合在 combo 释放时会卡死系统 workqueue（[issue #2356](https://github.com/zmkfirmware/zmk/issues/2356)、[issue #3100](https://github.com/zmkfirmware/zmk/issues/3100)），表现为 USB 保持连接但按键无输出。自定义行为经 `k_work_delayable` 异步分步执行，combo 释放不影响序列继续。

---

## 构建与刷写

使用 GitHub Actions 自动构建，使用 QMK Toolbox 刷入。
