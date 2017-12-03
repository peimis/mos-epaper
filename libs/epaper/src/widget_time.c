#include "mgos.h"
#include "widget.h"
#include "epdpaint.h"
#include "epaper.h"

#include "gfxfont.h"
#include "fonts/FreeSerif12pt7b.h"

// extern GFXfont FreeSerifBold9pt7b;

static void widget_time_render(struct widget_t *w, void *ev_data)
{
  sFONT *font = &Font24;
  char tmp_buff[32];
  int16_t text_width, text_height;

  time_t now = 2*3600 + time(0); // TZ=GMT+1
  struct tm* tm_info = gmtime(&now);
  int i;

  if (!w)
    return;

  mgos_ili9341_set_font(&FreeSerif12pt7b);

//  mgos_ili9341_set_fgcolor565(ILI9341_YELLOW);
  sprintf(tmp_buff, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
  printf("time: '%s'\n", tmp_buff);

  #define _WW   (((0+(strlen(tmp_buff) * font->Width))+7) & 0xF8)
  #define _HH   (0+(font->Height))

  mgos_epd_set_width(_WW);
  mgos_epd_set_height(_HH);

  text_width = mgos_ili9341_getStringWidth(tmp_buff);
  text_height = mgos_ili9341_getStringHeight(tmp_buff);


  mgos_epd_set_width(w->w);
  mgos_epd_set_height(w->h);
  printf("W=%d %d : %d %d\n", w->x, w->y, w->x + w->w, w->y + w->h);

  mgos_epd_print(text_width > (w->w) ? 0:(w->w-text_width)/2, text_height>(w->h) ? 0:(w->h-text_height)/2, tmp_buff);

  //
  mgos_epd_set_width(128);
  mgos_epd_set_height(32);

  mgos_epd_draw_filled_rectangle(0, 0, _WW, _HH, 1);
  mgos_epd_draw_string_at(0, 0, tmp_buff, font, 0);
  mgos_epd_pushFrameBuffer( mgos_epd_getFrameBuffer(), 40, 85, mgos_epd_get_width(), mgos_epd_get_height());

  mgos_epd_draw_filled_rectangle(0, 0, mgos_epd_get_width(), mgos_epd_get_height(), 1);
  mgos_epd_draw_rectangle(0, 0, mgos_epd_get_width()-1, mgos_epd_get_height()-1, 0);
  for (i=0; i<=(tm_info->tm_sec & 0x0F); i++) {
    mgos_epd_draw_filled_rectangle((8*i)+(i==0?3:0), 3, (8*i)+(i==15?4:5), 28, 0);
  }
  mgos_epd_pushFrameBuffer( mgos_epd_getFrameBuffer(), (100 - 64), 140, mgos_epd_get_width(), mgos_epd_get_height());

  mgos_epdUpdateNeeded();

  (void) ev_data;
}


void widget_time_ev(int ev, struct widget_t *w, void *ev_data)
{
  if (!w)
    return;

  switch(ev) {
    case EV_WIDGET_CREATE:
    case EV_WIDGET_DRAW:
    case EV_WIDGET_REDRAW:
    case EV_WIDGET_TIMER:
      widget_time_render(w, ev_data);
//      mgos_epd_display_frame();
      break;
    case EV_WIDGET_TOUCH_UP:
    case EV_WIDGET_TOUCH_DOWN:
    case EV_WIDGET_DESTROY:
    default: // EV_WIDGET_NONE
      break;
  }
}
