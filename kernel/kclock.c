#include <errno.h>

#include <kernel/kclock.h>

#define SECS_PER_YEAR	31536000
#define SECS_PER_DAY	86400

static time_t systolsec = 0;

uint8_t cmos_read(uint8_t reg)
{
	outb(IO_RTC0, reg);
	return inb(IO_RTC0+1);
}

void cmos_write(uint8_t reg, uint8_t datum)
{
	outb(IO_RTC0, reg);
	outb(IO_RTC0+1, datum);
}

void rtc_intr(void)
{
	if (systolsec)
		systolsec++;
	else
		systolsec = time(NULL);
}

#define islyear(y) (!(y % 400) || (!(y % 4) && (y % 100)))
#define bcd2bin(x) ((x >> 4) * 10 + (x & 0xF))
// days from y/01/01 to y/m/01.
static int mdays(int m, int y)
{
	int days = m * 30;
	int offd[12] = {0,1,-1,0,0,1,1,2,3,3,4,4};
	return days + offd[m-1] + (islyear(y) && m > 2 ? 1 : 0);
}
// leap year from 1970/01/01 to y/01/01.
static int lyears(int y)
{
	int ys = 0;
	for (int i=1970; i<y; i++)
		if (islyear(i))
			ys++;
	return ys;
}
// total seconds from 1970/01/01 00:00 to now.
// qemu seems to use 24-hour GWT and the values are BCD encoded.
time_t time(time_t *store)
{
	time_t tols, sec, min, hour, day, month, year;
	year = bcd2bin(cmos_read(CMOSR_YEAR));
	month = bcd2bin(cmos_read(CMOSR_MONTH));
	day = bcd2bin(cmos_read(CMOSR_DAY));
	hour = bcd2bin(cmos_read(CMOSR_HOUR));
	min = bcd2bin(cmos_read(CMOSR_MIN));
	sec = bcd2bin(cmos_read(CMOSR_SEC));
	tols = (year - 1970) * SECS_PER_YEAR + \
		(lyears(year) + mdays(month, year) + day) * SECS_PER_DAY + \
		hour * 60 * 60 + min * 60 + sec;
	if (store) *store = tols;
	return tols;
}

int stime(const time_t *t)
{
	return 0;
}