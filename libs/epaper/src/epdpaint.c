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


static int _width=0;
static int _height=0;
static uint8_t *_image=NULL;
static enum mgos_epd_rotate_t _rotate = ROTATE_0;

/**
 *  @brief: clear the image
 */
void mgos_epd_clear(const int colored)
{
	int x, y;

	LOG(LL_INFO, ("Clear display."));

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
			_image[(x + y * _width) / 8] |= (0x80 >> (x % 8));
		} else {
			_image[(x + y * _width) / 8] &= ~((0x80 >> (x % 8)));
		}
	} else {
		if (colored) {
			_image[(x + y * _width) / 8] &= ~(0x80 >> (x % 8));
		} else {
			_image[(x + y * _width) / 8] |= (0x80 >> (x % 8));
		}
	}
}

/**
 *  @brief: this draws a pixel by the coordinates
 */
void mgos_epd_draw_pixel(const int x, const int y, const int colored)
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
uint8_t* mgos_epd_get_image(void)
{
	return _image;
}

void mgos_epd_set_image(uint8_t* image)
{
	_image = image;
	LOG(LL_INFO, ("Set image=%p", image));
}

int mgos_epd_get_width(void)
{
	return _width;
}

void mgos_epd_set_width(const int width)
{
	_width = (width % 8) ? (width + 8 - (width % 8)) : width;
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

void mgos_epd_set_rotate(const enum mgos_epd_rotate_t rotate) {
	_rotate = rotate;

	LOG(LL_INFO, ("Rotate to %d", rotate));
}



/**
 *  @brief: this draws a charactor on the frame buffer but not refresh
 */
void mgos_epd_draw_char_at(const int x, const int y, const char ascii_char, const sFONT* const font, const int colored)
{
	unsigned int char_offset = (ascii_char - ' ') * font->Height * (font->Width / 8 + (font->Width % 8 ? 1 : 0));
	const uint8_t * ptr = &font->table[char_offset];
	int i, j;

	for (j = 0; j < font->Height; j++) {
		for (i = 0; i < font->Width; i++) {
			if ((*ptr) & (0x80 >> (i % 8))) {
				mgos_epd_draw_pixel(x + i, y + j, colored);
			}
			if ((i % 8) == 7) {
				ptr++;
			}
		}
		if ((font->Width % 8) != 0) {
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
void mgos_epd_draw_line(const int x0, const int y0, const int x1, const int y1, const int colored)
{
	/* Bresenham algorithm */
	int dx = (x1 - x0) >= 0 ? (x1 - x0) : (x0 - x1);
	int sx = (x0 < x1) ? 1 : -1;
	int dy = (y1 - y0) <= 0 ? (y1 - y0) : (y0 - y1);
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx + dy;
	int cx = x0, cy = y0;

	while ((cx != x1) && (cy != y1)) {
		mgos_epd_draw_pixel(cx, cy , colored);
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
		mgos_epd_draw_pixel(i, y, colored);
	}
}


/**
*  @brief: this draws a vertical line on the frame buffer
*/
void mgos_epd_draw_vertical_line(const int x, const int y, const int line_height, const int colored)
{
	int i;
	for (i = y; i < (y + line_height); i++) {
		mgos_epd_draw_pixel(x, i, colored);
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
void mgos_epd_draw_circle(const int x, const int y, const int radius, const int colored)
{
	/* Bresenham algorithm */
	int x_pos = -radius;
	int y_pos = 0;
	int err = 2 - 2 * radius;
	int e2;

	do {
		mgos_epd_draw_pixel(x - x_pos, y + y_pos, colored);
		mgos_epd_draw_pixel(x + x_pos, y + y_pos, colored);
		mgos_epd_draw_pixel(x + x_pos, y - y_pos, colored);
		mgos_epd_draw_pixel(x - x_pos, y - y_pos, colored);
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
void mgos_epd_draw_filled_circle(const int x, const int y, const int radius, const int colored)
{
	/* Bresenham algorithm */
	int x_pos = -radius;
	int y_pos = 0;
	int err = 2 - 2 * radius;
	int e2;

	do {
		mgos_epd_draw_pixel(x - x_pos, y + y_pos, colored);
		mgos_epd_draw_pixel(x + x_pos, y + y_pos, colored);
		mgos_epd_draw_pixel(x + x_pos, y - y_pos, colored);
		mgos_epd_draw_pixel(x - x_pos, y - y_pos, colored);
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

void mgos_epd_render_time(void)
{
	sFONT *font = &Font24;
	char tmp_buff[32];
	int i;
	time_t now = 2*3600 + time(0); // TZ=GMT+1
	struct tm* tm_info = gmtime(&now);

	sprintf(tmp_buff, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
	printf("time: '%s'\n", tmp_buff);

	#define _WW		(((0+(strlen(tmp_buff) * font->Width))+7) & 0xF8)
	#define _HH		(0+(font->Height))

	mgos_epd_set_width(_WW);
	mgos_epd_set_height(_HH);
//	mgos_epd_draw_filled_rectangle(0, 0, _WW, _HH, 0);
//	mgos_epd_draw_filled_rectangle(1, 1, _WW-2, _HH-2, 1);
	mgos_epd_draw_filled_rectangle(0, 0, _WW, _HH, 1);
	mgos_epd_draw_string_at(0, 0, tmp_buff, font, 0);
	mgos_epd_set_frame_memory( mgos_epd_get_image(), 40, 85, mgos_epd_get_width(), mgos_epd_get_height());

	mgos_epd_set_width(128);
	mgos_epd_set_height(32);
	mgos_epd_draw_filled_rectangle(0, 0, mgos_epd_get_width(), mgos_epd_get_height(), 1);
	mgos_epd_draw_rectangle(0, 0, mgos_epd_get_width()-1, mgos_epd_get_height()-1, 0);
	for (i=0; i<=(tm_info->tm_sec & 0x0F); i++) {
		mgos_epd_draw_filled_rectangle((8*i)+(i==0?3:0), 3, (8*i)+(i==15?4:5), 28, 0);
	}
	mgos_epd_set_frame_memory( mgos_epd_get_image(), (100 - 64), 140, mgos_epd_get_width(), mgos_epd_get_height());

	mgos_epd_set_width(128);
	mgos_epd_set_height(32);

}

/* END OF FILE */























