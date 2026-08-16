/*
 * Copyright (c) 2026 bvbhu
 * SPDX-License-Identifier: MIT
 *
 * rgb_matrix_settings.h — RGB Matrix 设置持久化模块声明
 * 监听 &rgb_ug 键码，修改灯光设置，并通过 ZMK settings 子系统持久化配置。
 */

#pragma once

#include "rgb_matrix.h"

/* ===== Settings 持久化 API ===== */
void rgb_matrix_settings_init(void);
void rgb_matrix_settings_save(void);