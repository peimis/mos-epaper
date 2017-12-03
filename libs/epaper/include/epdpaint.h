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
#include "fonts.h"

#ifndef _MGOS_LIBS_EPAPER_EPDPAINT_H
#define _MGOS_LIBS_EPAPER_EPDPAINT_H

// Display orientation
enum mgos_epd_rotate_t {
	ROTATE_0 = 0,
	ROTATE_90 = 1,
	ROTATE_180 = 2,
	ROTATE_270 = 3,
};
// Display orientation

#define swap(a, b) { int t=a; a=b; b=t; }

// Color inverse. 1 or 0 = set or reset a bit if set a colored pixel
#define IF_INVERT_COLOR     1

void mgos_epd_clear(const int colored);
void mgos_epd_draw_absolute_pixel(const int x, const int y, const int colored);

uint8_t * mgos_epd_get_image(void);

int mgos_epd_get_width(void);
void mgos_epd_set_width(const int width);
int mgos_epd_get_height(void);
void mgos_epd_set_height(const int height);
enum mgos_epd_rotate_t mgos_epd_get_rotate(void);
void mgos_epd_set_rotate(const enum mgos_epd_rotate_t rotate);

uint8_t* mgos_epd_getFrameBuffer(void);
void mgos_epd_setFrameBuffer(uint8_t* image);

void mgos_epd_draw_horizontal_line(const int x, const int y, const int line_width, const int colored);
void mgos_epd_draw_vertical_line(const int x, const int y, const int line_height, const int colored);
void mgos_epd_draw_rectangle(const int x0, const int y0, const int x1, const int y1, const int colored);
void mgos_epd_draw_filled_rectangle(const int x0, const int y0, const int x1, const int y1, const int colored);
void mgos_epd_drawCircle(const int x, const int y, const int radius, const int colored);


void mgos_epd_draw_char_at(const int x, const int y, const char ascii_char, const sFONT* const font, const int colored);
void mgos_epd_draw_string_at(const int x, const int y, const char* text, const sFONT* const font, const int colored);

void mgos_epd_drawFilledCircle(const int x, const int y, const int radius, const int colored);

/**
 *  @brief: this draws a pixel by the coordinates
 */
void mgos_epdDrawPixel(const int x, const int y, const int colored);
void mgos_epd_drawLine(const int x0, const int y0, const int x1, const int y1, const int colored);
void mgos_epd_drawRoundRect(int16_t x0, int16_t y0, uint16_t w, uint16_t h, uint16_t r, const int colored);

void mgos_epd_print(uint16_t x0, uint16_t y0, char *string);

#endif // _MGOS_LIBS_EPAPER_EPDPAINT_H

/* END OF FILE */

