#include <stacsos/console.h>
#include <stacsos/objects.h>

using namespace stacsos;

struct tod {
    u16 seconds, minutes, hours, dom, month, year;
};

int main(const char *cmdline)
{
	object *file = object::open("/dev/cmos-rtc0");
	
    if (!file) {
        console::get().write("Failed to open RTC device\n");
        return 1;
    }
    
    // Read the RTC data into a buffer
    tod timepoint;
    size_t bytes_read = file->pread(&timepoint, sizeof(timepoint), 0);
    
    if (bytes_read != sizeof(timepoint)) {
        console::get().write("Failed to read RTC data\n");
        return 1;
    }
    
    // Now you can use the timepoint data
    console::get().writef("Current time: %02d:%02d:%02d\n", 
                         timepoint.hours, timepoint.minutes, timepoint.seconds);
    console::get().writef("Current date: %02d/%02d/%04d\n", 
                         timepoint.dom, timepoint.month, timepoint.year);
    
    return 0;
}
