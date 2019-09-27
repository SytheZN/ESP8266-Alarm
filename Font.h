/*
  Font.h - Raster font base class.
  Copyright 2018, SytheZN, All rights reserved.
*/
#ifndef _Font_h
#define _Font_h

#include "Arduino.h"

class Font {
  public:
    virtual uint8_t    getCharWidth(char c) = 0;
    virtual uint8_t    getCharHeight(char c) = 0;
    virtual uint16_t   getCharDataStartOffset(char c) = 0;
    virtual uint16_t   getCharDataRowOffset(char c) = 0;
    virtual uint8_t    getCharGap(char c) = 0;
    virtual uint8_t    getCharDrop(char c) = 0;
    virtual byte       getByte(uint16_t index) = 0;
};

#endif
