/**
 *  @filename   :   epdpaint.cpp
 *  @brief      :   Paint tools
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     September 9 2017
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
#include <stdio.h>
#include <string.h>
#include "paint.h"
#include "esp_log.h"

static const char *TAG = "paint";


epd_paint_t *Paint_create(int width, int height)
{
    epd_paint_t *p = (epd_paint_t *)malloc(sizeof(epd_paint_t));
    p->width_memory = width;
    p->height_memory = height;
    p->rotate = ROTATE_0;
    p->bg_color = (color32_t) {
        .full = 0
    };
    p->color = (color32_t) {
        .ch.red = 20, .ch.green = 0, .ch.blue = 0
    };
    if (p->rotate == ROTATE_0 || p->rotate == ROTATE_180) {
        p->width = width;
        p->height = height;
    } else {
        p->width = height;
        p->height = width;
    }
    return p;
}

void Paint_delete(epd_paint_t *_paint)
{
    if (NULL == _paint) {
        return;
    }
    free(_paint);
}

/**
 *  @brief: clear the image
 */
void Paint_Clear(const epd_paint_t *paint)
{
    for (int x = 0; x < paint->width_memory; x++) {
        for (int y = 0; y < paint->height_memory; y++) {
            Paint_DrawAbsolutePixel(paint, x, y, paint->color);
        }
    }
}

/**
 *  @brief: this draws a pixel by absolute coordinates.
 *          this function won't be affected by the rotate parameter.
 */
void Paint_DrawAbsolutePixel(const epd_paint_t *paint, uint16_t x, uint16_t y, color32_t colored)
{
    void ws2812_write_pixel_xy(uint16_t _x, uint16_t _y, color32_t color);
    if (x >= paint->width || y >= paint->height) {
        // ESP_LOGE(TAG, "Cannot draw point at (%d,%d)", x, y);
        return;
    }
    ws2812_write_pixel_xy(x, y, colored);
}

/**
 *  @brief: this draws a pixel by the coordinates
 */
void Paint_DrawPixel(const epd_paint_t *paint, int x, int y, color32_t colored)
{
    int point_temp;
    int _width = paint->width_memory;
    int _height = paint->height_memory;

    switch (paint->rotate) {
    case ROTATE_0:
        // printf("x y %d %d\n", x, y);
        Paint_DrawAbsolutePixel(paint, x, y, colored);
        break;
    case ROTATE_90:
        point_temp = x;
        x = _width - 1 - y;
        y = point_temp;
        Paint_DrawAbsolutePixel(paint, x, y, colored);
        break;
    case ROTATE_180:
        x = _width - 1 - x;
        y = _height - 1 - y;
        Paint_DrawAbsolutePixel(paint, x, y, colored);
        break;
    case ROTATE_270:
        point_temp = x;
        x = y;
        y = _height - 1 - point_temp;
        Paint_DrawAbsolutePixel(paint, x, y, colored);
        break;
    default:
        // Handle unexpected rotation value
        break;
    }
}

/**
 *  @brief: this draws a charactor on the frame buffer but not refresh
 */
void Paint_DrawCharAt(const epd_paint_t *paint, int x, int y, char ascii_char, const sFONT *font)
{
    int i, j;
    uint8_t const *ptr;
    uint16_t font_len;
    font->get_char_mask(&ascii_char, &ptr, &font_len);

    for (j = 0; j < font->Height; j++) {
        for (i = 0; i < font->Width; i++) {
            if (*(ptr) & (0x80 >> (i % 8))) {
                Paint_DrawPixel(paint, x + i, y + j, paint->color);
            } else {
                Paint_DrawPixel(paint, x + i, y + j, paint->bg_color);
            }
            if (i % 8 == 7) {
                ptr++;
            }
        }
        if (font->Width % 8 != 0) {
            ptr++;
        }
    }
}

void Paint_DrawStringScroll(const epd_paint_t *paint, int x, int y, char ascii_char, const sFONT *font)
{
    int i, j;
    uint8_t const *ptr;
    uint16_t font_len;
    font->get_char_mask(&ascii_char, &ptr, &font_len);

    for (j = 0; j < font->Height; j++) {
        for (i = 0; i < font->Width; i++) {
            if (*(ptr) & (0x80 >> (i % 8))) {
                Paint_DrawPixel(paint, x + i, y + j, paint->color);
            } else {
                Paint_DrawPixel(paint, x + i, y + j, paint->bg_color);
            }
            if (i % 8 == 7) {
                ptr++;
            }
        }
        if (font->Width % 8 != 0) {
            ptr++;
        }
    }
}

/**
*  @brief: this displays a string on the frame buffer but not refresh
*/
void Paint_DrawStringAt(const epd_paint_t *paint, int x, int y, const char *text, const sFONT *font)
{
    const char *p_text = text;
    unsigned int counter = 0;
    int refcolumn = x;

    /* Send the string character by character on EPD */
    while (*p_text != 0) {
        /* Display one character on EPD */
        Paint_DrawCharAt(paint, refcolumn, y, *p_text, font);
        /* Decrement the column position by 16 */
        refcolumn += font->Width;
        /* Point on the next character */
        p_text++;
        counter++;
    }
}


/**
*  @brief: this displays a image on the frame buffer but not refresh
*/
void Paint_DrawImage(const epd_paint_t *paint, int x, int y, int width, int height, color32_t **img)
{

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            Paint_DrawPixel(paint, x + i, y + j, img[j][i]);
        }
    }
}

/**
*  @brief: this draws a line on the frame buffer
*/
void Paint_DrawLine(const epd_paint_t *paint, int x0, int y0, int x1, int y1)
{
    /* Bresenham algorithm */
    int dx = x1 - x0 >= 0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 - y0 <= 0 ? y1 - y0 : y0 - y1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while ((x0 != x1) && (y0 != y1)) {
        Paint_DrawPixel(paint, x0, y0, paint->color);
        if (2 * err >= dy) {
            err += dy;
            x0 += sx;
        }
        if (2 * err <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/**
*  @brief: this draws a horizontal line on the frame buffer
*/
void Paint_DrawHorizontalLine(const epd_paint_t *paint, int x, int y, int line_width)
{
    int i;
    for (i = x; i < x + line_width; i++) {
        Paint_DrawPixel(paint, i, y, paint->color);
    }
}

/**
*  @brief: this draws a vertical line on the frame buffer
*/
void Paint_DrawVerticalLine(const epd_paint_t *paint, int x, int y, int line_height)
{
    int i;
    for (i = y; i < y + line_height; i++) {
        Paint_DrawPixel(paint, x, i, paint->color);
    }
}

/**
*  @brief: this draws a rectangle
*/
void Paint_DrawRectangle(const epd_paint_t *paint, int x0, int y0, int x1, int y1)
{
    int min_x, min_y, max_x, max_y;
    min_x = x1 > x0 ? x0 : x1;
    max_x = x1 > x0 ? x1 : x0;
    min_y = y1 > y0 ? y0 : y1;
    max_y = y1 > y0 ? y1 : y0;

    Paint_DrawHorizontalLine(paint, min_x, min_y, max_x - min_x + 1);
    Paint_DrawHorizontalLine(paint, min_x, max_y, max_x - min_x + 1);
    Paint_DrawVerticalLine(paint, min_x, min_y, max_y - min_y + 1);
    Paint_DrawVerticalLine(paint, max_x, min_y, max_y - min_y + 1);
}

/**
*  @brief: this draws a filled rectangle
*/
void Paint_DrawFilledRectangle(const epd_paint_t *paint, int x0, int y0, int x1, int y1)
{
    int min_x, min_y, max_x, max_y;
    int i;
    min_x = x1 > x0 ? x0 : x1;
    max_x = x1 > x0 ? x1 : x0;
    min_y = y1 > y0 ? y0 : y1;
    max_y = y1 > y0 ? y1 : y0;

    for (i = min_x; i <= max_x; i++) {
        Paint_DrawVerticalLine(paint, i, min_y, max_y - min_y + 1);
    }
}

/**
*  @brief: this draws a circle
*/
void Paint_DrawCircle(const epd_paint_t *paint, int x, int y, int radius)
{
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        Paint_DrawPixel(paint, x - x_pos, y + y_pos, paint->color);
        Paint_DrawPixel(paint, x + x_pos, y + y_pos, paint->color);
        Paint_DrawPixel(paint, x + x_pos, y - y_pos, paint->color);
        Paint_DrawPixel(paint, x - x_pos, y - y_pos, paint->color);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if (-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}

/**
*  @brief: this draws a filled circle
*/
void Paint_DrawFilledCircle(const epd_paint_t *paint, int x, int y, int radius)
{
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        Paint_DrawPixel(paint, x - x_pos, y + y_pos, paint->color);
        Paint_DrawPixel(paint, x + x_pos, y + y_pos, paint->color);
        Paint_DrawPixel(paint, x + x_pos, y - y_pos, paint->color);
        Paint_DrawPixel(paint, x - x_pos, y - y_pos, paint->color);
        Paint_DrawHorizontalLine(paint, x + x_pos, y + y_pos, 2 * (-x_pos) + 1);
        Paint_DrawHorizontalLine(paint, x + x_pos, y - y_pos, 2 * (-x_pos) + 1);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if (-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}

/* END OF FILE */

