/*
  Font_8x8_Icons.h - 8 x 8px Raster Icon font.
  Copyright 2019, SytheZN, All rights reserved.
*/
#ifndef _Font_8x8_Icons_h
#define _Font_8x8_Icons_h

#include "Arduino.h"
#include "Font.h"

class Font_8x8_Icons: public Font {
  public:
    uint8_t    getCharWidth(char c);
    uint8_t    getCharHeight(char c);
    uint16_t   getCharDataStartOffset(char c);
    uint16_t   getCharDataRowOffset(char c);
    uint8_t    getCharGap(char c);
    uint8_t    getCharDrop(char c);
    byte       getByte(uint16_t index);
  private:
    static const byte fontData[];
};

enum struct Icons : char {
  Empty = ' ',
  Wifi = 'a',
  Network = 'b',
  ActivityLeft = 'c',
  ActivityRight = 'd',
  Computer = 'e',
  Bell = 'f',
  BellRinging = 'g',
  Button = 'h'
};

#endif

