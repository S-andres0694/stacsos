#include <stacsos/console.h>
#include <stacsos/ls.h>
#include <stacsos/memops.h>
#include <stacsos/objects.h>
#include <stacsos/string.h>
#include <stacsos/syscalls.h>

using namespace stacsos;

/**
 * Helper function to determine the maximum name length among directory entries.
 * This is used for formatting the output in long listing mode.
 *
 * @param result The ls_result structure containing directory entries.
 * @return The maximum length of the names of the entries.
 */
int get_max_name_length(const final_product &result)
{
	int max_len = 0;
	for (u64 i = 0; i < result.result.number_entries; i++) {
		const directory_entry &entry = result.entries[i];
		int len = memops::strlen(entry.name);
		if (len > max_len) {
			max_len = len;
		}
	}
	return max_len;
}

/**
 * Allows for an optimized printing for the -h flag (human-readable file sizes).
 * It does so by converting the size in bytes to a more human-friendly format.
 * Heavily inspired by https://stackoverflow.com/questions/281640/how-do-i-get-a-human-readable-file-size-in-bytes-abbreviation-using-net
 * answer by Adrian Hum.
 * The function has a small invariance within the precision. Because the OS does not seem to support floating-point operations, the precision margin of this
 * function is limited.
 * @param size The size in bytes.
 */

void print_human_readable_size(u64 size)
{
	const char *units[] = { "B", "KB", "MB", "GB", "TB" };
	u32 unit_index = 0;

	while (size >= 1024 && unit_index < 4) {
		size /= 1024;
		unit_index++;
	}
	console::get().writef("%llu %s\n", size, units[unit_index]);
}

/**
 * Function to print the results of the 'ls' syscall based on the provided flags.
 *
 * @param result The ls_result structure containing the results of the syscall.
 * @param flags The ls_flags structure indicating how to format the output.
 */

void ls::print_ls_result(const final_product &result, u8 flags)
{
	// This code is probably unreachable because I can guarantee that the syscall will always exist.
	if (result.result.code != syscall_result_code::ok) {
		console::get().write("Error: Unable to perform 'ls' operation.\nThere was an error resolving the necessary syscall.\n");
		return;
	}

	// Check the result code for the 'ls' operation and handle errors accordingly
	// to notify the user something went wrong.
	if (result.result.result_code != ls_result_code::ok) {
		switch (result.result.result_code) {
		case ls_result_code::directory_does_not_exist:
			console::get().write("Error: The specified directory does not exist.\n");
			break;
		case ls_result_code::file_was_passed:
			console::get().write("Error: A file path was provided instead of a directory path.\n");
			break;
		case ls_result_code::directory_empty:
			console::get().write("The specified directory is empty.\n");
			break;
		case ls_result_code::unsupported_filesystem:
			console::get().write("Error: The filesystem of the specified directory is not supported by the 'ls' command.\n");
			break;
		default:
			console::get().write("Error: An unknown error occurred during the 'ls' operation.\n");
			break;
		}
		return;
	}

	// Print the directory entries based on the flags provided
	for (u64 i = 0; i < result.result.number_entries; i++) {
		const directory_entry &entry = result.entries[i];

		if (!(flags & LS_FLAG_ALL_FILES)) {
			// Skip hidden files (those starting with a dot) if the ALL_FILES flag is not set
			if ((entry.name[0] == '.' && entry.name[1] == '\0') || (entry.name[0] == '.' && entry.name[1] == '.' && entry.name[2] == '\0')) {
				continue;
			}
		}

		// Long listing format based on the specification sheet.
		if (flags & LS_FLAG_LONG_LISTING) {
			// Determine the maximum name length for formatting
			// Also set up column widths accordingly.
			int max_name_length = get_max_name_length(result);

			// Minimum spaces between columns. Just hardcoded it and chose
			// because it looks reasonable.
			int minimum_padding = 6;

			// The width should be the maximum name length plus some padding.
			int NAME_COLUMN_WIDTH = max_name_length + minimum_padding;

			char type_char = (entry.type == fs_node_kind::directory) ? 'D' : 'F';

			console::get().writef("[%c] %s", type_char, entry.name);

			// Calculate padding dynamically based on the name length.
			int name_len = memops::strlen(entry.name);
			int padding = NAME_COLUMN_WIDTH - name_len;

			// Make sure there's at least one space of padding, regardless of the
			// name length.
			if (padding < 1) {
				padding = 1;
			}

			// Add the padding dynamically.
			for (int j = 0; j < padding; j++) {
				console::get().write(" ");
			}

			if (entry.type == fs_node_kind::file) {
				if (flags & LS_FLAG_HUMAN_READABLE) {
					print_human_readable_size(entry.size);
				} else {
					console::get().writef("%llu\n", entry.size);
				}
			} else {
				console::get().writef("\n");
			}
		} else {
			// Simple listing format
			console::get().writef("%s\n", entry.name);
		}
	}
}

int main(const char *cmdline)
{
	// Validate command line arguments
	if (cmdline == nullptr || cmdline[0] == '\0') {
		console::get().write("Usage: ls [-l] [-a] [-R] [-h] <path>\n");
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
			case 'r':
				flags |= LS_FLAG_RECURSIVE;
				break;
			default:
				console::get().write("error: usage: ls [-l] [-a] [-R] [-h] <path>\n");
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
		console::get().write("error: usage: ls [-l] [-a] [-R] [-h] <path>\n");
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