#include <stacsos/kernel/arch/x86/pio.h>
#include <stacsos/kernel/dev/misc/cmos-rtc.h>
#define CURRENT_YEAR 2025

using namespace stacsos::kernel::dev;
using namespace stacsos::kernel::dev::misc;
using namespace stacsos::kernel::arch::x86;

int century_register = 0x00;

unsigned char second;
unsigned char minute;
unsigned char hour;
unsigned char day;
unsigned char month;
unsigned int year;

int get_update_in_progress_flag()
{
	ioports::cmos_select::write8(0x0A);
	return (ioports::cmos_data::read8() & 0x80);
}

unsigned char get_RTC_register(int reg)
{
	ioports::cmos_select::write8(reg);
	return ioports::cmos_data::read8();
}

device_class cmos_rtc::cmos_rtc_device_class(rtc::rtc_device_class, "cmos-rtc");

rtc_timepoint cmos_rtc::read_timepoint()
{
	rtc_timepoint timepoint;
	unsigned char registerB;
	unsigned char century;
	unsigned char last_second;
	unsigned char last_minute;
	unsigned char last_hour;
	unsigned char last_day;
	unsigned char last_month;
	unsigned char last_year;
	unsigned char last_century;

	while (get_update_in_progress_flag())
		; // Make sure an update isn't in progress
	second = get_RTC_register(0x00);
	minute = get_RTC_register(0x02);
	hour = get_RTC_register(0x04);
	day = get_RTC_register(0x07);
	month = get_RTC_register(0x08);
	year = get_RTC_register(0x09);
	if (century_register != 0) {
		century = get_RTC_register(century_register);
	}

	do {
		last_second = second;
		last_minute = minute;
		last_hour = hour;
		last_day = day;
		last_month = month;
		last_year = year;
		last_century = century;

		while (get_update_in_progress_flag())
			; // Make sure an update isn't in progress
		second = get_RTC_register(0x00);
		minute = get_RTC_register(0x02);
		hour = get_RTC_register(0x04);
		day = get_RTC_register(0x07);
		month = get_RTC_register(0x08);
		year = get_RTC_register(0x09);
		if (century_register != 0) {
			century = get_RTC_register(century_register);
		}
	} while ((last_second != second) || (last_minute != minute) || (last_hour != hour) || (last_day != day) || (last_month != month) || (last_year != year)
		|| (last_century != century));

	registerB = get_RTC_register(0x0B);

	// Convert BCD to binary values if necessary

	if (!(registerB & 0x04)) {
		second = (second & 0x0F) + ((second / 16) * 10);
		minute = (minute & 0x0F) + ((minute / 16) * 10);
		hour = ((hour & 0x0F) + (((hour & 0x70) / 16) * 10)) | (hour & 0x80);
		day = (day & 0x0F) + ((day / 16) * 10);
		month = (month & 0x0F) + ((month / 16) * 10);
		year = (year & 0x0F) + ((year / 16) * 10);
		if (century_register != 0) {
			century = (century & 0x0F) + ((century / 16) * 10);
		}
	}

	// Convert 12 hour clock to 24 hour clock if necessary

	if (!(registerB & 0x02) && (hour & 0x80)) {
		hour = ((hour & 0x7F) + 12) % 24;
	}

	// Calculate the full (4-digit) year

	if (century_register != 0) {
		year += century * 100;
	} else {
		year += (CURRENT_YEAR / 100) * 100;
		if (year < CURRENT_YEAR)
			year += 100;
	}

	// For some reason it is an hour less lol
	if (hour == 23) {
		hour = 0;
	} else {
		hour++;
	}

	// Calculate the full (4-digit) year
	timepoint.seconds = second;
	timepoint.minutes = minute;
	timepoint.hours = hour; 
	timepoint.day_of_month = day;
	timepoint.month = month;
	timepoint.year = year;

	return timepoint;
}