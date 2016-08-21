#include <stdint.h>
#include <mips.h>
#include <malta.h>
#include <rtc.h>
#include <clock.h>
/* http://geezer.osdevbrasil.net/temp/rtc.txt */

#define RTC_ADDR_R *(volatile uint8_t*)(MIPS_PHYS_TO_KSEG1(MALTA_RTC_ADDR))
#define RTC_DATA_R *(volatile uint8_t*)(MIPS_PHYS_TO_KSEG1(MALTA_RTC_DATA))

#define REG_B_DM 4
#define REG_B_24 2

void rtc_init() {
  RTC_ADDR_R = 0xb;
  RTC_DATA_R = RTC_DATA_R | REG_B_DM | REG_B_24;
}

void rtc_read(rtc_time_t *t) {
  RTC_ADDR_R = 0; t->sec = RTC_DATA_R;
  RTC_ADDR_R = 2; t->min = RTC_DATA_R;
  RTC_ADDR_R = 4; t->hour = RTC_DATA_R;
  RTC_ADDR_R = 6; t->wday = RTC_DATA_R;
  RTC_ADDR_R = 7; t->mday = RTC_DATA_R;
  RTC_ADDR_R = 8; t->month = RTC_DATA_R;
  RTC_ADDR_R = 9; t->year = RTC_DATA_R + 2000;
}

void mdelay (unsigned msec) {
  unsigned now = clock_get_ms();
  unsigned final = now + msec;
  while (final > clock_get_ms());
}

#ifdef _KERNELSPACE
#include <libkern.h>

/*
 * Delays for at least the given number of milliseconds. May not be
 * nanosecond-accurate.
 */

int main() {
  while (1) {
    rtc_time_t rtc;
    rtc_read(&rtc);

    kprintf("Time is %02d:%02d:%02d\n", rtc.hour, rtc.min, rtc.sec);

    mdelay(1000);
  }

  return 0;
}
#endif

