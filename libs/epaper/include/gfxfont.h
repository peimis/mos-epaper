#ifndef _MGOS_LIBS_EPAPER_GFXFONT_H_
#define _MGOS_LIBS_EPAPER_GFXFONT_H_



typedef struct { // Data stored PER GLYPH
  uint16_t bitmapOffset;     // Pointer into GFXfont->bitmap
  uint8_t  width, height;    // Bitmap dimensions in pixels
  uint8_t  xAdvance;         // Distance to advance cursor (x axis)
  int8_t   xOffset, yOffset; // Dist from cursor pos to UL corner
} GFXglyph;

typedef struct { // Data stored for FONT AS A WHOLE:
  uint8_t  *bitmap;      // Glyph bitmaps, concatenated
  GFXglyph *glyph;       // Glyph array
  uint8_t   first, last; // ASCII extents
  uint8_t   yAdvance;    // Newline distance (y axis)
      
  // Added(pimvanpelt) for framebuffer rendering.
  int8_t    font_height; // Maximum per-glyph height
  int8_t    font_width;  // Maximum per-glyph width
  int8_t    font_min_xOffset;  // Left-most glyph xOffset
  int8_t    font_min_yOffset;  // Left-most glyph yOffset
} GFXfont;

enum GFXfont_t {
  GFXFONT_NONE      = 0,
  GFXFONT_INTERNAL  = 1,
  GFXFONT_FILE      = 2,
};


// Internal functions -- do not use
uint16_t ili9341_print_fillPixelLine(char *string, uint8_t line, uint8_t *buf, uint8_t color);

// Fonts and Printing:
bool mgos_ili9341_set_font(GFXfont *f);
void mgos_ili9341_print(uint16_t x0, uint16_t y0, char *s);
uint16_t mgos_ili9341_getStringWidth(char *string);
uint16_t mgos_ili9341_getStringHeight(char *string);

#endif
