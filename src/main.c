/**
 *  @filename   :   epd1in54-demo.ino
 *  @brief      :   1.54inch e-paper display demo
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     September 5 2017
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

#define COLORED     0
#define UNCOLORED   1

/**
  * Due to RAM not enough in Arduino UNO, a frame buffer is not allowed.
  * In this case, a smaller image buffer is allocated and you have to 
  * update a partial display several times.
  * 1 byte = 8 pixels, therefore you have to set 8*N pixels at a time.
  */
uint8_t imagebuffer[1024];

unsigned long time_start_ms;
unsigned long time_now_s;


void epaper_demo(void)
{
	int display;
	// put your setup code here, to run once:
	mgos_epd_set_image(imagebuffer);

	/** 
	*  there are 2 memory areas embedded in the e-paper display
	*  and once the display is refreshed, the memory area will be auto-toggled,
	*  i.e. the next action of SetFrameMemory will set the other memory area
	*  therefore you have to clear the frame memory twice.
	*/
	mgos_epd_clear_frame_memory(0xFF);   // bit set = white, bit reset = black
	mgos_epd_display_frame();
	mgos_epd_clear_frame_memory(0xFF);   // bit set = white, bit reset = black
	mgos_epd_display_frame();

	for (display=0; display<2; display++)
	{
	mgos_epd_set_rotate(ROTATE_0);
	mgos_epd_set_width(200);
	mgos_epd_set_height(24);

	/* For simplicity, the arguments are explicit numerical coordinates */
	mgos_epd_clear(COLORED);
	mgos_epd_draw_string_at(2, 2, "Hello Mongoose!", &Font20, UNCOLORED);
	mgos_epd_set_frame_memory(mgos_epd_get_image(), 0, 10, mgos_epd_get_width(), mgos_epd_get_height());

	mgos_epd_clear(UNCOLORED);
	const char *istr = "MOS epaper lib";
	mgos_epd_draw_string_at(100 - (Font16.Width*strlen(istr)/2), 4, istr, &Font16, COLORED);
	mgos_epd_set_frame_memory(mgos_epd_get_image(), 0, 30, mgos_epd_get_width(), mgos_epd_get_height());

	mgos_epd_clear(UNCOLORED);
	mgos_epd_draw_string_at(5, 4, "* Using native mgos_spi.h", &Font12, COLORED);
	mgos_epd_set_frame_memory(mgos_epd_get_image(), 0, 50, mgos_epd_get_width(), mgos_epd_get_height());

/*
	mgos_epd_set_width(64);
	mgos_epd_set_height(64);

	mgos_epd_clear(UNCOLORED);
	mgos_epd_draw_rectangle(0, 0, 40, 45, COLORED);
	mgos_epd_draw_line(0, 0, 40, 45, COLORED);
	mgos_epd_draw_line(40, 0, 0, 45, COLORED);
	mgos_epd_set_frame_memory(mgos_epd_get_image(), 16, 80, mgos_epd_get_width(), mgos_epd_get_height());

	mgos_epd_clear(UNCOLORED);
	mgos_epd_draw_circle(32, 32, 20, COLORED);
	mgos_epd_set_frame_memory(mgos_epd_get_image(), 120, 70, mgos_epd_get_width(), mgos_epd_get_height());

	mgos_epd_clear(UNCOLORED);
	mgos_epd_draw_filled_rectangle(0, 0, 46, 45, COLORED);
	mgos_epd_set_frame_memory(mgos_epd_get_image(), 16, 130, mgos_epd_get_width(), mgos_epd_get_height());

	mgos_epd_clear(UNCOLORED);
	mgos_epd_draw_filled_circle(32, 32, 25, COLORED);
	mgos_epd_set_frame_memory(mgos_epd_get_image(), 100, 130, mgos_epd_get_width(), mgos_epd_get_height());
*/
	mgos_epd_display_frame();
	}

	if (0 != mgos_epd_display_init(PARTIAL_UPDATE)) {
		LOG(LL_ERROR, ("Could not initialize ePaper display"));
		return;
	}

	/** 
	*  there are 2 memory areas embedded in the e-paper display
	*  and once the display is refreshed, the memory area will be auto-toggled,
	*  i.e. the next action of SetFrameMemory will set the other memory area
	*  therefore you have to set the frame memory and refresh the display twice.
	*/

	time_start_ms = ((int)mg_time())/1000;
}


void epaper_timer_cb(void *param)
{
	mgos_epd_render_time();
	mgos_epd_display_frame();
}


//
//
enum mgos_app_init_result mgos_app_init(void)
{

  	printf("App init\n");

	epaper_demo();

	mgos_set_timer(943, 1, epaper_timer_cb, NULL);

	return MGOS_APP_INIT_SUCCESS;
}
