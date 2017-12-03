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

#include "mgos.h"
#include "epaper.h"
#include "epdpaint.h"

#include "gfxfont.h"


struct {
	int x0;
	int y0;
	int x1;
	int y1;
	int color;
} window;

static int _width=0;
static int _height=0;
static struct window clip_window;

static uint8_t *_image=NULL;
static enum mgos_epd_rotate_t _rotate = ROTATE_0;

//
//
static void mgos_epd_drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, const int colored);


/**
 *  @brief: clear the image
 */
void mgos_epd_clear(const int colored)
{
	int x, y;

	for (x = 0; x < _width; x++) {
		for (y = 0; y < _height; y++) {
			mgos_epd_draw_absolute_pixel(x, y, colored);
		}
	}
}

/**
 *  @brief: this draws a pixel by absolute coordinates.
 *          this function won't be affected by the rotate parameter.
 */
void mgos_epd_draw_absolute_pixel(const int x, const int y, const int colored)
{
	if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) {
		return;
	}

	if (IF_INVERT_COLOR) {
		if (colored) {
			_image[(x + y * _width) >> 3] |= (0x80 >> (x & 0x07));
		} else {
			_image[(x + y * _width) >> 3] &= ~((0x80 >> (x & 0x07)));
		}
	} else {
		if (colored) {
			_image[(x + y * _width) >> 3] &= ~(0x80 >> (x & 0x07));
		} else {
			_image[(x + y * _width) >> 3] |= (0x80 >> (x & 0x07));
		}
	}
}

/**
 *  @brief: this draws a pixel by the coordinates
 */
void mgos_epdDrawPixel(const int x, const int y, const int colored)
{
	int rotated_x = x, rotated_y = y;

	if ((x < 0) || (y < 0)) {
		return;
	}

	if (_rotate == ROTATE_0) {
		if ((x >= _width) || (y >= _height)) {
			return;
		}
	} else if (_rotate == ROTATE_90) {
		if ((x >= _height) || (y >= _width)) {
		  return;
		}
		rotated_x = _width - y;
		rotated_y = x;
	} else if (_rotate == ROTATE_180) {
		if ((x >= _width) || (y >= _height)) {
		  return;
		}
		rotated_x = _width - x;
		rotated_y = _height - y;
	} else if (_rotate == ROTATE_270) {
		if ((x >= _height) || (y >= _width)) {
		  return;
		}
		rotated_x = y;
		rotated_y = _height - x;
	}
	mgos_epd_draw_absolute_pixel(rotated_x, rotated_y, colored);
}


/**
 *  @brief: Getters and Setters
 */
uint8_t* mgos_epd_getFrameBuffer(void)
{
	return _image;
}

void mgos_epd_setFrameBuffer(uint8_t* image)
{
	_image = image;
}

int mgos_epd_get_width(void)
{
	return _width;
}

void mgos_epd_set_width(const int width)
{
	_width = (width & 0x07) ? (width + 8 - (width & 0x07)) : width;
}

int mgos_epd_get_height(void)
{
	return _height;
}

void mgos_epd_set_height(const int height)
{
	_height = height;
}

enum mgos_epd_rotate_t mgos_epd_get_rotate(void)
{
	return _rotate;
}

void mgos_epd_set_rotate(const enum mgos_epd_rotate_t rotate)
{
	_rotate = rotate;
}



void mgos_epd_print(uint16_t x0, uint16_t y0, char *string)
{
	uint16_t pixelline_width=0;
	uint8_t *linebuffer;
	uint16_t lines;
	uint8_t pixelcolor = 0;

	pixelline_width = (mgos_ili9341_getStringWidth(string)+ 7) & ~0x07;
	if (pixelline_width==0) {
		LOG(LL_ERROR, ("getStringWidth returned 0 -- is the font set?"));
		return;
	}

	lines = mgos_ili9341_getStringHeight(string);
	if (lines==0) {
		LOG(LL_ERROR, ("getStringHeight returned 0 -- is the font set?"));
		return;
	}
	LOG(LL_INFO, ("string='%s' at (%d,%d), width=%u height=%u", string, x0, y0, pixelline_width, lines));

	linebuffer = calloc( 1+pixelline_width, 1);

	if (!linebuffer) {
		LOG(LL_ERROR, ("could not malloc for string='%s' at (%d,%d), width=%u height=%u", string, x0, y0, pixelline_width, lines));
		return;
	}

	for (int line=0; line<mgos_ili9341_getStringHeight(string); line++) {
		int ret;
		for (int i=0; i<pixelline_width; i++) {
			linebuffer[i] = (pixelcolor == 0) ? 0xFF : 0;
		}

		ret = ili9341_print_fillPixelLine(string, line, linebuffer, pixelcolor);
		ret = (ret + 7) & ~0x07;

		if (ret != pixelline_width) {
			LOG(LL_ERROR, ("ili9341_getStringPixelLine returned %d, but we expected %d", ret, pixelline_width));
		}
		mgos_epd_pushFrameBuffer(linebuffer, x0, y0+line, pixelline_width, 1);
//		ili9341_send_pixels(x0, y0+line, x0+pixelline_width-1, y0+line, (uint8_t *)pixelline, pixelline_width*sizeof(uint16_t));
	}
	free(linebuffer);
}


/**
 *  @brief: this draws a charactor on the frame buffer but not refresh
 */
void mgos_epd_draw_char_at(const int x, const int y, const char ascii_char, const sFONT* const font, const int colored)
{
	unsigned int char_offset = (ascii_char - ' ') * font->Height * ((font->Width >> 3) + ((font->Width & 0x07) ? 1 : 0));
	const uint8_t * ptr = &font->table[char_offset];
	int i, j;

	for (j = 0; j < font->Height; j++) {
		for (i = 0; i < font->Width; i++) {
			if ((*ptr) & (0x80 >> (i & 0x07))) {
				mgos_epdDrawPixel(x + i, y + j, colored);
			}
			if ((i & 0x07) == 7) {
				ptr++;
			}
		}
		if ((font->Width & 0x07) != 0) {
			ptr++;
		}
	}
}


/**
*  @brief: this displays a string on the frame buffer but not refresh
*/
void mgos_epd_draw_string_at(const int x, const int y, const char* text, const sFONT* const font, const int colored)
{
	const char* p_text = text;
	int counter = 0;
	int refcolumn = x;
	
	/* Send the string character by character on EPD */
	while (*p_text != 0) {
		/* Display one character on EPD */
		mgos_epd_draw_char_at(refcolumn, y, *p_text, font, colored);
		/* Decrement the column position by 16 */
		refcolumn += font->Width;
		/* Point on the next character */
		p_text++;
		counter++;
	}
}


/**
*  @brief: this draws a line on the frame buffer
*/
void mgos_epd_drawLine(const int x0, const int y0, const int x1, const int y1, const int colored)
{
	/* Bresenham algorithm */
	int dx = (x1 - x0) >= 0 ? (x1 - x0) : (x0 - x1);
	int sx = (x0 < x1) ? 1 : -1;
	int dy = (y1 - y0) <= 0 ? (y1 - y0) : (y0 - y1);
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx + dy;
	int cx = x0, cy = y0;

	while ((cx != x1) && (cy != y1)) {
		mgos_epdDrawPixel(cx, cy , colored);
		if (2 * err >= dy) {     
			err += dy;
			cx += sx;
		}
		if (2 * err <= dx) {
			err += dx; 
			cy += sy;
		}
	}
}


/**
*  @brief: this draws a horizontal line on the frame buffer
*/
void mgos_epd_draw_horizontal_line(const int x, const int y, const int line_width, const int colored)
{
	int i;
	for (i = x; i < (x + line_width); i++) {
		mgos_epdDrawPixel(i, y, colored);
	}
}


/**
*  @brief: this draws a vertical line on the frame buffer
*/
void mgos_epd_draw_vertical_line(const int x, const int y, const int line_height, const int colored)
{
	int i;
	for (i = y; i < (y + line_height); i++) {
		mgos_epdDrawPixel(x, i, colored);
	}
}


/**
*  @brief: this draws a rectangle
*/
void mgos_epd_draw_rectangle(const int x0, const int y0, const int x1, const int y1, const int colored)
{
	int min_x, min_y, max_x, max_y;

	min_x = (x1 > x0) ? x0 : x1;
	max_x = (x1 > x0) ? x1 : x0;
	min_y = (y1 > y0) ? y0 : y1;
	max_y = (y1 > y0) ? y1 : y0;
	
	mgos_epd_draw_horizontal_line(min_x, min_y, max_x - min_x + 1, colored);
	mgos_epd_draw_horizontal_line(min_x, max_y, max_x - min_x + 1, colored);
	mgos_epd_draw_vertical_line(min_x, min_y, max_y - min_y + 1, colored);
	mgos_epd_draw_vertical_line(max_x, min_y, max_y - min_y + 1, colored);
}


/**
*  @brief: this draws a filled rectangle
*/
void mgos_epd_draw_filled_rectangle(const int x0, const int y0, const int x1, const int y1, const int colored)
{
	int min_x, min_y, max_x, max_y;
	int i;
	min_x = (x1 > x0) ? x0 : x1;
	max_x = (x1 > x0) ? x1 : x0;
	min_y = (y1 > y0) ? y0 : y1;
	max_y = (y1 > y0) ? y1 : y0;
	
	for (i = min_x; i <= max_x; i++) {
	  mgos_epd_draw_vertical_line(i, min_y, max_y - min_y + 1, colored);
	}
}


/**
*  @brief: this draws a circle
*/
void mgos_epd_drawCircle(const int x, const int y, const int radius, const int colored)
{
	/* Bresenham algorithm */
	int x_pos = -radius;
	int y_pos = 0;
	int err = 2 - 2 * radius;
	int e2;

	do {
		mgos_epdDrawPixel(x - x_pos, y + y_pos, colored);
		mgos_epdDrawPixel(x + x_pos, y + y_pos, colored);
		mgos_epdDrawPixel(x + x_pos, y - y_pos, colored);
		mgos_epdDrawPixel(x - x_pos, y - y_pos, colored);
		e2 = err;
		if (e2 <= y_pos) {
			err += ++y_pos * 2 + 1;
			if(-x_pos == y_pos && e2 <= x_pos) {
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
void mgos_epd_drawFilledCircle(const int x, const int y, const int radius, const int colored)
{
	/* Bresenham algorithm */
	int x_pos = -radius;
	int y_pos = 0;
	int err = 2 - 2 * radius;
	int e2;

	do {
		mgos_epdDrawPixel(x - x_pos, y + y_pos, colored);
		mgos_epdDrawPixel(x + x_pos, y + y_pos, colored);
		mgos_epdDrawPixel(x + x_pos, y - y_pos, colored);
		mgos_epdDrawPixel(x - x_pos, y - y_pos, colored);
		mgos_epd_draw_horizontal_line(x + x_pos, y + y_pos, 2 * (-x_pos) + 1, colored);
		mgos_epd_draw_horizontal_line(x + x_pos, y - y_pos, 2 * (-x_pos) + 1, colored);
		e2 = err;
		if (e2 <= y_pos) {
			err += ++y_pos * 2 + 1;
			if(-x_pos == y_pos && e2 <= x_pos) {
				e2 = 0;
			}
		}
		if (e2 > x_pos) {
			err += ++x_pos * 2 + 1;
		}
	} while (x_pos <= 0);
}

// ---------------------------------------------------------------------------
//

static void mgos_epd_drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, const int colored)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}

		x++;
		ddF_x += 2;
		f += ddF_x;

		if (cornername & 0x1) {
			mgos_epdDrawPixel(x0 - y, y0 - x, colored);
			mgos_epdDrawPixel(x0 - x, y0 - y, colored);
		}
		if (cornername & 0x2) {
			mgos_epdDrawPixel(x0 + x, y0 - y, colored);
			mgos_epdDrawPixel(x0 + y, y0 - x, colored);
		}
		if (cornername & 0x4) {
			mgos_epdDrawPixel(x0 + x, y0 + y, colored);
			mgos_epdDrawPixel(x0 + y, y0 + x, colored);
		}
		if (cornername & 0x8) {
			mgos_epdDrawPixel(x0 - y, y0 + x, colored);
			mgos_epdDrawPixel(x0 - x, y0 + y, colored);
		}
	}
}


void mgos_epd_drawRoundRect(int16_t x0, int16_t y0, uint16_t w, uint16_t h, uint16_t r, const int colored)
{
	// draw the straight edges
	mgos_epd_drawLine(x0+r, y0, x0+w-r, y0, colored);         // Top
	mgos_epd_drawLine(x0+r, y0+h-1, x0+w-r, y0+h-1, colored); // Bottom
	mgos_epd_drawLine(x0, y0+r, x0, y0+h-r, colored);         // Left
	mgos_epd_drawLine(x0+w-1, y0+r, x0+w-1, y0+h-r, colored); // Right

	// draw four corners
	mgos_epd_drawCircleHelper(x0+r, y0+r, r, 1, colored);          // Top Left
	mgos_epd_drawCircleHelper(x0+w-r-1, y0+r, r, 2, colored);      // Top Right
	mgos_epd_drawCircleHelper(x0+r, y0+h-r-1, r, 8, colored);      // Bottom Left
	mgos_epd_drawCircleHelper(x0+w-r-1, y0+h-r-1, r, 4, colored);  // Bottom Right
}


void mgos_epd_drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, int colored)
{
  mgos_epd_drawLine(x0, y0, x1, y1, colored);
  mgos_epd_drawLine(x1, y1, x2, y2, colored);
  mgos_epd_drawLine(x2, y2, x0, y0, colored);
}


void mgos_epd_fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, int colored)
{
  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    mgos_epd_drawLine(a, y0, b+1, y0, colored);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
  int32_t
    sa   = 0,
    sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    mgos_epd_drawLine(a, y, b+1, y, colored);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    mgos_epd_drawLine(a, y, b+1, y, colored);
  }
}

// ---------------------------------------------------------------------------
//

/* END OF FILE */























