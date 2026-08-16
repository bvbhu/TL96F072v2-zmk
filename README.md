# TL96F072v2 ZMK 固件 & QMK RGB Matrix

本项目全部使用 AI 编写。

适用于 **洛塔洛塔洛** 制作的 **TL96F072v2** PCB 的 ZMK 固件，复刻了 QMK RGB Matrix，编写了一些自定义行为。

<img width="480" height="181" alt="键盘图片1" src="https://github.com/user-attachments/assets/43bbfab6-0a9f-4a59-9983-aa52a0bc6fb5" />
<img width="480" height="187" alt="键盘图片2" src="https://github.com/user-attachments/assets/d82f63c4-31b0-4a66-9504-26c5ff799f2e" />

---

### RGB Matrix

使用自定义 RGB 控制器取代 ZMK 内置 `rgb_underglow`，复刻 QMK RGB Matrix。支持全部灯效，灯效/HSV/速度/开关状态自动保存至 flash，空闲可自动熄灯。详细说明请参见 [src/rgb_matrix/README.md](src/rgb_matrix/README.md)，移植参考请参见 [RGB Matrix移植参考.md](<RGB Matrix移植参考.md>)。

---

键位图推荐使用 [ZMK Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) 查看。

---

## 自定义行为

使用 `zmk,behavior-bvbhu` 自定义行为驱动，配合统一事件监听器实现以下功能：

- **`&bvbhu 0` — Lead 序列匹配**：由 Combo 触发，激活后拦截所有按键构建序列，支持前缀匹配，全数字序列自动依次输出，空格、回车、Lead 键或超时（2s）终止。预设序列见 `src/behaviors/behavior_bvbhu.c` 的 `lead_entries`。
- **`&bvbhu_right_space` — 右空格**：按住临时激活层 1（Lower），释放后恢复；单击空格、双击回车、三击退格回车、四击 `"- "` + Ctrl+V。
- **`&f13` — 状态复位 / Alt+F4**：tap-dance 行为，单击切回 0 层并执行 `&bvbhu 1`（关 CapsLock/ScrollLock、开 NumLock），双击发送 Alt+F4。
- **`&kp RG(P)` — 优化 Win+P 投影**：延迟 2s 释放 Win 键。

---

## 构建与刷写

使用 GitHub Actions 自动构建，使用 QMK Toolbox 刷入。
