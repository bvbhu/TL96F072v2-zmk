/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 FastLED
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * lib8tion.c — FastLED lib8tion 数学函数移植
 * 来自 FastLED 项目，提供 sin8/cos8/random8/scale8 等基础运算。
 */
#include "lib8tion.h"

#define FASTLED_RAND16_2053 ((uint16_t)(2053))
#define FASTLED_RAND16_13849 ((uint16_t)(13849))

static uint16_t rand16seed = 0x1357;

uint8_t qadd8(uint8_t i, uint8_t j)
{
	uint16_t t = (uint16_t)i + j;
	if(t > 255) t = 255;
	return (uint8_t)t;
}

uint8_t qsub8(uint8_t i, uint8_t j)
{
	return (i >= j) ? (uint8_t)(i - j) : 0;
}

int8_t abs8(int8_t i)
{
	return (i < 0) ? (int8_t)(-i) : i;
}

/* FastLED sqrt16: 输入 uint16_t，返回 uint8_t（二分搜索） */
uint8_t sqrt16(uint16_t x)
{
	if(x <= 1)
	{
		return (uint8_t)x;
	}
	uint8_t low = 1;
	uint8_t hi, mid;
	if(x > 7904)
	{
		hi = 255;
	}
	else
	{
		hi = (uint8_t)(x >> 5) + 8;
	}
	do
	{
		mid = (uint8_t)((low + hi) >> 1);
		if((uint16_t)(mid * mid) > x)
		{
			hi = (uint8_t)(mid - 1);
		}
		else
		{
			if(mid == 255)
			{
				return 255;
			}
			low = (uint8_t)(mid + 1);
		}
	} while(hi >= low);
	return (uint8_t)(low - 1);
}

/* ===== scale8 ===== */

uint8_t scale8(uint8_t i, uint8_t scale)
{
	return (uint8_t)(((uint16_t)i * (uint16_t)scale) >> 8);
}

uint16_t scale16by8(uint16_t i, uint8_t scale)
{
	return (uint16_t)(((uint32_t)i * (uint32_t)scale) >> 8);
}

/* ===== random8 ===== */

uint8_t random8(void)
{
	rand16seed = (uint16_t)(rand16seed * FASTLED_RAND16_2053 + FASTLED_RAND16_13849);
	return (uint8_t)(((uint8_t)(rand16seed & 0xFF)) + ((uint8_t)(rand16seed >> 8)));
}

uint16_t random16(void)
{
	rand16seed = (uint16_t)(rand16seed * FASTLED_RAND16_2053 + FASTLED_RAND16_13849);
	return rand16seed;
}

uint8_t random8_max(uint8_t lim)
{
	uint8_t r = random8();
	return (uint8_t)(((uint16_t)r * lim) >> 8);
}

uint8_t random8_min_max(uint8_t min, uint8_t lim)
{
	uint8_t delta = (uint8_t)(lim - min);
	return (uint8_t)(random8_max(delta) + min);
}

static const uint8_t b_m16_interleave[8] = { 0, 49, 49, 41, 90, 27, 117, 10 };

uint8_t sin8(uint8_t theta)
{
	uint8_t offset = theta;
	if(theta & 0x40)
	{
		offset = (uint8_t)(255 - offset);
	}
	offset &= 0x3F;					   /* 0..63 */
	uint8_t secoffset = offset & 0x0F; /* 0..15 */
	if(theta & 0x40) secoffset++;
	uint8_t section = offset >> 4; /* 0..3 */
	uint8_t s2 = section * 2;
	uint8_t b = b_m16_interleave[s2];
	uint8_t m16 = b_m16_interleave[s2 + 1];
	uint8_t mx = (uint8_t)((m16 * secoffset) >> 4);
	int8_t y = (int8_t)(mx + b);
	if(theta & 0x80) y = (int8_t)(-y);
	y += 128;
	return (uint8_t)y;
}

uint8_t cos8(uint8_t theta)
{
	return sin8((uint8_t)(theta + 64));
}

uint8_t atan2_8(int16_t dy, int16_t dx)
{
	if(dy == 0)
	{
		if(dx >= 0)
			return 0;
		else
			return 128;
	}
	int16_t abs_y = (dy > 0) ? dy : (int16_t)(-dy);
	int8_t a;
	if(dx >= 0)
		a = (int8_t)(32 - (32 * (dx - abs_y) / (dx + abs_y)));
	else
		a = (int8_t)(96 - (32 * (dx + abs_y) / (abs_y - dx)));
	if(dy < 0) return (uint8_t)(-a);
	return (uint8_t)a;
}