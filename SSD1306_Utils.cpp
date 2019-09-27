#include "SSD1306_Utils.h"
#include "Arduino.h"

uint8_t SSD1306_Utils::write_char(SSD1306* screen, const uint8_t x, const uint8_t y, Font* f, const char c)
{
  const auto w = f->getCharWidth(c),
    h = f->getCharHeight(c),
    gap = f->getCharGap(c),
    drop = f->getCharDrop(c);
  const auto xi = f->getCharDataStartOffset(c),
    yi = f->getCharDataRowOffset(c);

  for (uint8_t cx = 0; cx < w + gap; cx++)
    for (uint8_t cy = 0; cy < h + gap + drop; cy++)
    {
      if (cx >= w || cy >= h + drop || cy < drop || c == 32)
      {
        // blank space
        screen->set_pixel(x + cx, y + cy, false);
      }
      else
      {
        const auto byte_num = ((yi * (cy - drop)) + xi + cx) / 8;
        const auto bit_num = 7 - (((yi * cy) + xi + cx) % 8);
        screen->set_pixel(x + cx, y + cy, 0x01 & (f->getByte(byte_num) >> bit_num));
      }
    }
  return w + gap;
}

void SSD1306_Utils::write_string(SSD1306* screen, uint8_t x, const uint8_t y, Font* f, String str)
{
  for (auto i : str)
  {
    const auto shift = write_char(screen, x, y, f, i);
    x += shift;
  }
}
