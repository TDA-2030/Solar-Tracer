/**
 *  @filename   :   epdpaint.h
 *  @brief      :   Header file for epdpaint.cpp
 *  @author     :   Yehui from Waveshare
 *  
 *  Copyright (C) Waveshare     July 28 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef EPDPAINT_H
#define EPDPAINT_H

#include <stdint.h>
#include "font/fonts.h"
#include "color.h"


#ifdef __cplusplus 
extern "C" {
#endif

// Display orientation
#define ROTATE_0            0
#define ROTATE_90           1
#define ROTATE_180          2
#define ROTATE_270          3

// Color inverse. 1 or 0 = set or reset a bit if set a colored pixel
#define IF_INVERT_COLOR     0



typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t width_memory;
    uint16_t height_memory;
    uint16_t rotate;
    color32_t bg_color;
    color32_t color;

} epd_paint_t;

epd_paint_t *Paint_create(int width, int height);
void Paint_delete(epd_paint_t *_paint);
void Paint_Clear(const epd_paint_t *paint);
void Paint_DrawAbsolutePixel(const epd_paint_t *paint, uint16_t x, uint16_t y, color32_t colored);
void Paint_DrawPixel(const epd_paint_t *paint, int x, int y, color32_t colored);
void Paint_DrawCharAt(const epd_paint_t *paint, int x, int y, char ascii_char, const sFONT* font);
void Paint_DrawStringAt(const epd_paint_t *paint, int x, int y, const char* text, const sFONT* font);
void Paint_DrawLine(const epd_paint_t *paint, int x0, int y0, int x1, int y1);
void Paint_DrawHorizontalLine(const epd_paint_t *paint, int x, int y, int width);
void Paint_DrawVerticalLine(const epd_paint_t *paint, int x, int y, int height);
void Paint_DrawRectangle(const epd_paint_t *paint, int x0, int y0, int x1, int y1);
void Paint_DrawFilledRectangle(const epd_paint_t *paint, int x0, int y0, int x1, int y1);
void Paint_DrawCircle(const epd_paint_t *paint, int x, int y, int radius);
void Paint_DrawFilledCircle(const epd_paint_t *paint, int x, int y, int radius);
void Paint_DrawImage(const epd_paint_t *paint, int x, int y, int width, int height, color32_t **img);


#ifdef __cplusplus 
}
#endif

#endif

/* END OF FILE */

