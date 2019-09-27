#ifndef _SSD1306_Utils_h
#define _SSD1306_Utils_h

#include "Arduino.h"
#include "SSD1306_SWI2C.h"
#include "Font.h"

class SSD1306_Utils
{
  public:
    static uint8_t write_char(SSD1306* screen, uint8_t x, uint8_t y, Font* f, char c);
    static void write_string(SSD1306* screen, uint8_t x, uint8_t y, Font* f, String str);
};

#endif
