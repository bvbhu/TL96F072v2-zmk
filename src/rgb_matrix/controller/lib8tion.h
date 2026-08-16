/*
 * Copyright (c) 2013 FastLED
 * SPDX-License-Identifier: MIT
 *
 * FastLED lib8tion 数学函数声明
 */

#pragma once

#include <stdint.h>

uint8_t qadd8(uint8_t i, uint8_t j);
uint8_t qsub8(uint8_t i, uint8_t j);
int8_t abs8(int8_t i);
uint8_t sqrt16(uint16_t x);
uint8_t scale8(uint8_t i, uint8_t scale);
uint16_t scale16by8(uint16_t i, uint8_t scale);
uint8_t random8(void);
uint16_t random16(void);
uint8_t random8_max(uint8_t lim);
uint8_t random8_min_max(uint8_t min, uint8_t lim);
uint8_t sin8(uint8_t theta);
uint8_t cos8(uint8_t theta);
uint8_t atan2_8(int16_t dy, int16_t dx);