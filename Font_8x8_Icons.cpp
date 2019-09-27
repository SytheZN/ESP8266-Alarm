/*
  Font_8x8_Icons.cpp - 8 x 8px Raster Icon font.
  Copyright 2019, SytheZN, All rights reserved.
*/
#include "Font_8x8_Icons.h"

const PROGMEM byte Font_8x8_Icons::fontData [] = {
  0b00111100, 0b00111000, 0b00000000, 0b00000000, 0b11111111, 0b00011000, 0b00011000, 0b00000000,
  0b01000010, 0b00111000, 0b00000100, 0b00100000, 0b10000001, 0b00111100, 0b00111100, 0b00000000,
  0b10111101, 0b00010000, 0b00001001, 0b10010000, 0b10000001, 0b01011110, 0b01011110, 0b00000000,
  0b01000010, 0b01111100, 0b00001010, 0b01010000, 0b10000001, 0b01011110, 0b01011110, 0b00000000,
  0b00011000, 0b01000100, 0b00001001, 0b10010000, 0b11111111, 0b01011110, 0b01011110, 0b00111100,
  0b00011000, 0b11101110, 0b00000100, 0b00100000, 0b11111111, 0b11111111, 0b11111111, 0b11111111,
  0b00000000, 0b11101110, 0b00000000, 0b00000000, 0b00011000, 0b00000000, 0b00000000, 0b00000000,
  0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00111100, 0b00011000, 0b01100110, 0b00000000,
};

byte Font_8x8_Icons::getByte(uint16_t index) {
  return pgm_read_byte_near(fontData + index);
}

uint8_t Font_8x8_Icons::getCharWidth(char c) {
  switch ((Icons)c){
    case Icons::Empty:
    case Icons::Wifi:
    case Icons::Network:
    case Icons::ActivityLeft:
    case Icons::ActivityRight:
    case Icons::Computer:
    case Icons::Bell:
    case Icons::BellRinging:
    case Icons::Button:
      return 8;

    default:
      break;
  }

  // default
  return 0;
}

uint8_t Font_8x8_Icons::getCharHeight(char c) {
  return 8;
}

uint16_t Font_8x8_Icons::getCharDataStartOffset(char c) {
  switch ((Icons)c){
    case Icons::Wifi:
      return 0;
    case Icons::Network:
      return 8;
    case Icons::ActivityLeft:
      return 16;
    case Icons::ActivityRight:
      return 24;
    case Icons::Computer:
      return 32;
    case Icons::Bell:
      return 40;
    case Icons::BellRinging:
      return 48;
    case Icons::Button:
      return 56;

    default:
      break;
  }
  
  return 0;
}

uint16_t Font_8x8_Icons::getCharDataRowOffset(char c) {
  return (8 * 8); // width of dataset (bits to skip per Ypx)
}

uint8_t Font_8x8_Icons::getCharGap(char c) {
  return 0;
}

uint8_t Font_8x8_Icons::getCharDrop(char c) {
  return 0;
}

