#include "mgos.h"
#include "gfxfont.h"



static GFXfont *s_font = NULL;
static enum GFXfont_t s_font_type = GFXFONT_NONE;



static bool ili9341_analyzeFont(GFXfont *f) {
	int chars = f->last - f->first;
	int maxHeight=0, minyo=0, maxWidth=0, maxAdvance=0, minxo=0, maxxo=0, maxhyo=0, minhyo=0, maxyo=0, maxwxo=0, minwxo=0;

	for (int i=0; i<chars; i++) {
		GFXglyph *glyph  = f->glyph+i;
		uint8_t  w  = glyph->width;
		uint8_t  h  = glyph->height;
		int8_t   xo = glyph->xOffset;
		int8_t   xa = glyph->xAdvance;
		int8_t   yo = glyph->yOffset;

		LOG(LL_DEBUG, ("char=0x%02x '%c' w=%d h=%d xOffset=%d yOffset=%d xAdvance=%d", i+f->first, i+f->first, w, h, xo, yo, xa));
		if (h+yo>maxhyo) maxhyo=h+yo;
		if (-(h+yo)>minhyo) minhyo=h+yo;
		if (w+xo>maxwxo) maxwxo=w+xo;
		if (-(w+xo)>minwxo) minwxo=w+xo;

		if (yo<minyo) minyo=yo;
		if (yo>maxyo) maxyo=yo;
		if (xo<minxo) minxo=xo;
		if (xo>maxxo) maxxo=xo;
		if (xo>maxxo) maxxo=xo;
		if (h>maxHeight) maxHeight=h;
		if (w>maxWidth) maxWidth=w;
		if (xa>maxAdvance) maxAdvance=xa;
	}
	LOG(LL_DEBUG, ("maxHeight=%d maxWidth=%d maxAdvance=%d minxo=%d maxxo=%d minyo=%d maxyo=%d maxhyo=%d, minhyo=%d", maxHeight, maxWidth, maxAdvance, minxo, maxxo, minyo, maxyo, maxhyo, minhyo));
	f->font_height = maxhyo-minyo;
	f->font_width = maxAdvance>maxWidth?maxAdvance:maxWidth;
	f->font_min_xOffset = minxo;
	f->font_min_yOffset = minyo;
	LOG(LL_INFO, ("Fontsize: HxW = %dx%d; min_xOffset=%d, min_yOffset=%d, there are %d chars in this font", f->font_height,f->font_width, f->font_min_xOffset, f->font_min_yOffset, chars));

	return true;
}


// -----------------------------------------------------------------------------
//
uint16_t ili9341_print_fillPixelLine(char *string, uint8_t line, uint8_t *buf, uint8_t color)
{
	uint16_t pixelline_width=0;
	uint8_t  *bitmap;
	uint16_t xx=0;
	uint16_t bitpos, bytepos;
 
	if (!s_font || !string || !buf)
		return 0;
	bitmap = s_font->bitmap;

//  LOG(LL_DEBUG, ("Line %d of string '%s'", line, string));
	for (uint16_t i=0; i<strlen(string); i++)
	{
		if(string[i]<s_font->first || string[i]>s_font->last) {
			LOG(LL_DEBUG, ("String character 0x%02x is not in font, replacing with ' '", i));
			string[i]=' ';
		}
		GFXglyph *glyph  = s_font->glyph+(string[i]-s_font->first);
		uint8_t  w  = glyph->width;
		uint8_t  h  = glyph->height;
		int8_t   xo = glyph->xOffset;
		int8_t   yo = glyph->yOffset;
		int8_t   xa = glyph->xAdvance;
		uint16_t bo = glyph->bitmapOffset;
		uint16_t bo_bitoffset = glyph->width*(line- (yo-s_font->font_min_yOffset));
		uint8_t  bits;
		uint16_t fl=0, ll=0;

//    LOG(LL_DEBUG, ("char=0x%02x '%c' w=%d h=%d xOffset=%d yOffset=%d xAdvance=%d bo=%d bo_bitoffset=%d", string[i], string[i], w, h, xo, yo, xa, bo, bo_bitoffset));
		if (xo<0 && pixelline_width==0) {
//      LOG(LL_DEBUG, ("First glyph '%c' has negative xOffset, adding %d to length", string[i], -xo));
			pixelline_width=-xo;
		}

		// First and last line of char
		fl = yo-s_font->font_min_yOffset;
		ll = fl+h-1;

		if (line < fl || line > ll) {
//      LOG(LL_DEBUG, ("Glyph '%c' goes from line [%d..%d], skipping", string[i], fl, ll));
		} else {
			bo+=(bo_bitoffset/8);
			bo_bitoffset%=8;
//      	LOG(LL_DEBUG, ("Glyph '%c' goes from [%d..%d] on lines [%d..%d], data bo=%d%%%d for %d bits", string[i], pixelline_width+xo, pixelline_width+xo+w-1, fl, ll, bo, bo_bitoffset, w));

			bits=bitmap[bo];
			bits<<=bo_bitoffset;

			for (xx=pixelline_width+xo; xx<pixelline_width+xo+w; xx++) {
				bytepos = xx >> 3;
				bitpos = 7-(xx & 0x07);
				if(bits & 0x80) {
					// buf[xx] = color;
					if( !color ) {
						buf[bytepos] &= ~(1 << bitpos);
					} else {
						buf[bytepos] |= (1 << bitpos);
					}
				}

				bits <<= 1;
//        LOG(LL_DEBUG, ("bo=%d%%%d (%02x) pixel=%d", bo, bo_bitoffset, bit, xx));
				if (!(++bo_bitoffset % 8)) {
					bits = bitmap[++bo];
				}
			}
		}
		pixelline_width+=xa;
		if (string[i+1]==0 && w>xa+xo) {
//      LOG(LL_DEBUG, ("Last glyph '%c' is wider than it advances, adding %d to length", string[i], w-xa+xo));
			pixelline_width+=(w-xa+xo);
		}
	}
//  LOG(LL_DEBUG, ("pixelline_width returns %d", pixelline_width));
	return pixelline_width;
}


// -----------------------------------------------------------------------------
//
uint16_t mgos_ili9341_getStringHeight(char *string)
{
	if (!s_font || !string || strlen(string) == 0)
		return 0;
	return s_font->font_height;
}


// -----------------------------------------------------------------------------
//
uint16_t mgos_ili9341_getStringWidth(char *string)
{
	uint16_t pixelline_width=0;
 
	if (!s_font || !string)
		return 0;

	for (uint16_t i=0; i<strlen(string); i++) {
		if(string[i]<s_font->first || string[i]>s_font->last) {
			LOG(LL_WARN, ("String character 0x%02x is not in font, replacing with ' '", i));
			string[i]=' ';
		}
		GFXglyph *glyph  = s_font->glyph+(string[i]-s_font->first);
		uint8_t  w  = glyph->width;
		int8_t   xo = glyph->xOffset;
		int8_t   xa = glyph->xAdvance;
//    LOG(LL_DEBUG, ("char=0x%02x '%c' w=%d xOffset=%d xAdvance=%d", string[i], string[i], w, xo, xa));

		if (xo<0 && pixelline_width==0) {
//      LOG(LL_DEBUG, ("First character has negative xOffset, adding %d to length", -xo));
			pixelline_width=-xo;
		}
//    LOG(LL_DEBUG, ("char goes from row [%d..%d], cursor at %d", pixelline_width+xo, pixelline_width+xo+w-1, pixelline_width));
		pixelline_width+=xa;
		if (string[i+1]==0 && w>xa+xo) {
			LOG(LL_DEBUG, ("Last character is wider than it advances, adding %d to length", w-xa+xo));
			pixelline_width+=(w-xa+xo);
		}
	}

	return pixelline_width;
}



bool mgos_ili9341_set_font(GFXfont *f)
{
	if (s_font_type == GFXFONT_FILE && s_font) free(s_font);

	if (!f) {
		s_font_type = GFXFONT_NONE;
		s_font = f;
		return false;
	}
	if (f->font_width==0 && f->font_height==0)
		ili9341_analyzeFont(f);
	s_font = f;
	s_font_type = GFXFONT_INTERNAL;
	return true;
}
