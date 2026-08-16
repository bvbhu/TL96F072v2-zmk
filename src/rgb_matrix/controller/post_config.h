// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// clang-format off

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <dt-bindings/zmk/matrix_transform.h>

// framebuffer
#if defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP) || \
    defined(ENABLE_RGB_MATRIX_DIGITAL_RAIN)
#    define RGB_MATRIX_FRAMEBUFFER_EFFECTS
#endif

// reactive
#if defined(ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE) || \
    defined(ENABLE_RGB_MATRIX_SOLID_REACTIVE) || \
    defined(ENABLE_RGB_MATRIX_SOLID_REACTIVE_WIDE) || \
    defined(ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTIWIDE) || \
    defined(ENABLE_RGB_MATRIX_SOLID_REACTIVE_CROSS) || \
    defined(ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTICROSS) || \
    defined(ENABLE_RGB_MATRIX_SOLID_REACTIVE_NEXUS) || \
    defined(ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTINEXUS) || \
    defined(ENABLE_RGB_MATRIX_SPLASH) || \
    defined(ENABLE_RGB_MATRIX_MULTISPLASH) || \
    defined(ENABLE_RGB_MATRIX_SOLID_SPLASH) || \
    defined(ENABLE_RGB_MATRIX_SOLID_REACTIVE_NEXUS) || \
    defined(ENABLE_RGB_MATRIX_SOLID_MULTISPLASH)
#    define RGB_MATRIX_KEYPRESSES
#endif

/* ===== position→(row,col) 映射宏 (供反应式/帧缓冲灯效使用) ===== */
#if defined(RGB_MATRIX_KEYPRESSES) || defined(RGB_MATRIX_KEYRELEASES) || \
    (defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP))
#    if !DT_HAS_CHOSEN(zmk_matrix_transform)
#        undef RGB_MATRIX_KEYPRESSES
#        undef RGB_MATRIX_KEYRELEASES
#        undef RGB_MATRIX_FRAMEBUFFER_EFFECTS
#    else
#        define ZMK_RGB_MT_NODE DT_CHOSEN(zmk_matrix_transform)
#        define ZMK_RGB_MT_LEN DT_PROP_LEN(ZMK_RGB_MT_NODE, map)
#        define ZMK_RGB_POS_RC_ENTRY(idx, _)                              \
            { (uint8_t)KT_ROW(DT_PROP_BY_IDX(ZMK_RGB_MT_NODE, map, idx)), \
              (uint8_t)KT_COL(DT_PROP_BY_IDX(ZMK_RGB_MT_NODE, map, idx)) }
#        define RGB_MATRIX_POS_TO_RC_LEN ZMK_RGB_MT_LEN
#        define RGB_MATRIX_POS_TO_RC_MAP \
            { LISTIFY(ZMK_RGB_MT_LEN, ZMK_RGB_POS_RC_ENTRY, (, ), 0) }
#    endif
#endif
