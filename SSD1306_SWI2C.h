/*
  SSD1306_SWI2C.h - Display driver for SSD1306 display, icnludes TWI & Display Buffer
  Copyright 2018, SytheZN, All rights reserved.
*/
#ifndef _SSD1306_h
#define _SSD1306_h

#include "Arduino.h"

#define ADDR_W               0b00111100
#define ADDR_R               0b00111101
#define CB_DATA              0b01000000
#define CB_CTRL              0b00000000

#define CMD_DISP_ON          0xAF
#define CMD_DISP_OFF         0xAE
#define CMD_TEST_ON          0xA5
#define CMD_TEST_OFF         0xA4
#define CMD_INVERT_OFF       0xA6
#define CMD_INVERT_ON        0xA7
#define CMD_CHARGE_PUMP      0x8D
#define DATA_CHARGE_PUMP_ON  0x14
#define DATA_CHARGE_PUMP_OFF 0x10
#define CMD_CONTRAST         0x81
#define CMD_PRECHARGE        0xD9
#define CMD_ADDRMODE         0x20
#define CMD_ADDRMODE_H       0x00
#define CMD_ADDRMODE_V       0x01
#define CMD_ADDRMODE_P       0x02
#define CMD_ADDR_HVCOL       0x21
#define CMD_ADDR_HVPAGE      0x22


class SSD1306
{
  public:
            SSD1306(int sda, int scl);
    void    set_pixel(uint8_t x, uint8_t y, bool state);
    void    clear_buffer();
    void    blank();
    void    refresh();
    
  private:
    void    twi_init();
    void    twi_start();
    void    twi_stop();
    void    twi_send(byte val);
    void    display_init();
};

#endif

