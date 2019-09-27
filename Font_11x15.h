/*
  Font_11x15.h - 11 x 15px Raster font.
  Copyright 2018, SytheZN, All rights reserved.
*/
#ifndef _Font_11x15_h
#define _Font_11x15_h

#include "Arduino.h"
#include "Font.h"

class Font_11x15: public Font {
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

#endif

