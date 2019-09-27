// see https://github.com/adafruit/Adafruit_NeoPixel
#include "Arduino.h"
#include "eagle_soc.h"

#define BP_SWI 0x00008000

#define GPIO_ON_ADDR 0x60000304
#define GPIO_OFF_ADDR 0x60000308

// 80MHz
#define CYCLES_TOTAL 96
#define CYCLES_ONE 50
#define CYCLES_ZERO 24

// 160MHz
// #define CYCLES_TOTAL 196
// #define CYCLES_ONE 100
// #define CYCLES_ZERO 49


static uint32_t _getCycleCount(void) __attribute__((always_inline));
static inline uint32_t _getCycleCount(void) {
  uint32_t ccount;
  __asm__ __volatile__("rsr %0,ccount":"=a" (ccount));
  return ccount;
}

void ICACHE_RAM_ATTR swi_write_ext(uint8_t *data, uint16_t len, uint8_t repeat)
{
  uint32_t startTime, c;

  for (uint16_t i = 0; i < (len * repeat); i++)
  {
    for (uint8_t mask = 0x80; mask; mask >>= 1)
    {
      if (data[i % len] & mask)
      {
        // one = on -> 0.6us -> off -> 0.6us
        while(((c = _getCycleCount()) - startTime) < CYCLES_TOTAL);
        WRITE_PERI_REG(GPIO_ON_ADDR, BP_SWI);
        startTime = c;
        while(((c = _getCycleCount()) - startTime) < CYCLES_ONE);
        WRITE_PERI_REG(GPIO_OFF_ADDR, BP_SWI);
      }
      else
      {
        // zero = on -> 0.3us -> off -> 0.9us
        while(((c = _getCycleCount()) - startTime) < CYCLES_TOTAL);
        WRITE_PERI_REG(GPIO_ON_ADDR, BP_SWI);
        startTime = c;
        while(((c = _getCycleCount()) - startTime) < CYCLES_ZERO);
        WRITE_PERI_REG(GPIO_OFF_ADDR, BP_SWI);
      }
    }
  }
};