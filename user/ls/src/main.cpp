#include <stacsos/console.h>

using namespace stacsos;

int main (const char *cmdline){
    if(cmdline == nullptr || cmdline[0] == '\0'){
        console::get().write("Usage: ls <path>\n");
        return 1;
    }

    if (cmdline[0] != '/') {
        console::get().write("All passed paths must be absolute\n");
        return 1;
    }

    // TODO: Should do the syscall here. 
    console::get().writef("The given path is: %s\n", cmdline);
	return 0;
}