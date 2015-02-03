#ifndef __KCLOCK_H__
#define __KCLOCK_H__

#include <x86.h>

#include <kernel/kernel.h>

#define	CMOSR_SEC	0x00
#define CMOSR_MIN	0x02
#define CMOSR_HOUR	0x04
#define CMOSR_DAY	0x07
#define CMOSR_MONTH	0x08
#define CMOSR_YEAR	0x09

uint8_t cmos_read(uint8_t reg);
void cmos_write(uint8_t reg, uint8_t datum);

void rtc_intr(void);

time_t time(time_t *store);

#endif /* !__KCLOCK_H__ */
