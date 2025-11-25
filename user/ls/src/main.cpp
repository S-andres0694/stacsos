#include <stacsos/ls.h>

using namespace stacsos;

int main(const char *cmdline)
{
	// Validate command line arguments
	if (cmdline == nullptr || cmdline[0] == '\0') {
		console::get().write("Usage: ls [-l] [-a] [-S] [-N] [-h] <path>\n");
		return 1;
	}

	// Make sure that the path is less than the maximum length.
	size_t cmdline_length = memops::strlen(cmdline);
	if (cmdline_length >= MAX_PATHNAME_LENGTH) {
		console::get().write("error: The provided path is too long.\n");
		return 1;
	}

	// Check if the flags are set.
	u8 flags = 0;

	// Parse flags
	if (*cmdline == '-') {
		cmdline++; // skip '-'
		while (*cmdline && *cmdline != ' ') {
			switch (*cmdline) {
			case 'l':
				flags |= LS_FLAG_LONG_LISTING;
				break;
			case 'a':
				flags |= LS_FLAG_ALL_FILES;
				break;
			case 'h':
				flags |= LS_FLAG_HUMAN_READABLE;
				break;
			case 'S':
				flags |= LS_FLAG_SORT_BY_SIZE;
				break;
			case 'N':
				flags |= LS_FLAG_SORT_BY_NAME;
				break;
			default:
				console::get().write("error: usage: ls [-l] [-a] [-S] [-N] [-h] <path>\n");
				return 1;
			}
			cmdline++; // move to the next flag character
		}
	}

	// Skip any spaces between flags and the path
	while (*cmdline == ' ') {
		cmdline++;
	}

	// Ensures that the path is not empty after checking the flags
	if (*cmdline == '\0') {
		console::get().write("error: usage: ls [-l] [-a] [-S] [-N] [-h] <path>\n");
		return 1;
	}

	// There's a path (or what looks like it). Ensure path is absolute since we do not support relative paths nor current directory.
	if (cmdline[0] != '/') {
		console::get().write("error: All passed paths must be absolute\n");
		return 1;
	}

	// The -h flag requires the -l flag to be set as well.
	if (flags & LS_FLAG_HUMAN_READABLE && !(flags & LS_FLAG_LONG_LISTING)) {
		console::get().write("error: The -h flag requires the -l flag to be set as well.\n");
		return 1;
	}

	// Both sorting flags cannot be set at the same time.
	if ((flags & LS_FLAG_SORT_BY_NAME) && (flags & LS_FLAG_SORT_BY_SIZE)) {
		console::get().write("error: The -N and -S flags cannot be set at the same time.\n");
		return 1;
	}

	// Perform the 'ls' syscall to write to the device
	ls::ls_syscall_wrapper(cmdline, flags);

	object *file = object::open("/dev/ls-device0");

	if (!file) {
		console::get().write("Error: Unable to open /dev/ls-device0\n");
		return 1;
	}

	final_product result;
	size_t bytes_read = file->pread(&result, sizeof(final_product), 0);

	if (bytes_read != sizeof(result)) {
		console::get().write("Error: Unable to read ls result from /dev/ls-device0\n");
		return 1;
	}

	ls::print_ls_result(result, flags);
	return 0;
}