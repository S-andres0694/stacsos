#pragma once
#include <stacsos/console.h>
#include <stacsos/memops.h>
#include <stacsos/objects.h>
#include <stacsos/string.h>
#include <stacsos/syscalls.h>
#include <stacsos/user-syscall.h>

namespace stacsos {
/**
 * Class which wraps over the native 'ls' syscall and provides utility functions
 * to print the results.
 */
class ls {
public:
	static void ls_syscall_wrapper(const char *path, u8 flags);

	static ls_result new_ls_result();

	/**
	 * Helper function to determine the maximum name length among directory entries.
	 * This is used for formatting the output in long listing mode.
	 *
	 * @param result The ls_result structure containing directory entries.
	 * @return The maximum length of the names of the entries.
	 */
	static u32 get_max_name_length(const final_product &result)
	{
		u32 max_len = 0;
		for (u64 i = 0; i < result.result.number_entries; i++) {
			const directory_entry &entry = result.entries[i];
			u32 len = memops::strlen(entry.name);
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

	static void print_human_readable_size(u64 size)
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
	 * Simple bubble sort implementation to sort directory entries.
	 * This is used to sort the entries based on name or size.
	 * @param entries The array of directory entries to sort.
	 * @param count The number of entries in the array.
	 * @param mode The sorting mode (by name or by size).
	 */

	static void bubble_sort(directory_entry *entries, u64 count, sort_mode mode)
	{
		if (count <= 1)
			return;

		for (u32 i = 0; i < count - 1; i++) {
			bool swapped = false;

			for (u32 j = 0; j < count - i - 1; j++) {
				bool should_swap = false;

				if (mode == SORT_BY_NAME) {
					if (memops::strcmp(entries[j].name, entries[j + 1].name) > 0) {
						should_swap = true;
					}
				} else if (mode == SORT_BY_SIZE) {
					if (entries[j].size > entries[j + 1].size) {
						should_swap = true;
					}
				}

				if (should_swap) {
					directory_entry tmp = entries[j];
					entries[j] = entries[j + 1];
					entries[j + 1] = tmp;
					swapped = true;
				}
			}

			// Optimization: stop if already sorted
			if (!swapped)
				break;
		}
	}

	/**
	 * Function to print the results of the 'ls' syscall based on the provided flags.
	 *
	 * @param result The ls_result structure containing the results of the syscall.
	 * @param flags The ls_flags structure indicating how to format the output.
	 */

	static void print_ls_result(const final_product &result, u8 flags)
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

		// Sort the entries if sorting flags are provided
		if (flags & LS_FLAG_SORT_BY_NAME) {
			bubble_sort(const_cast<directory_entry *>(result.entries), (result.result.number_entries), SORT_BY_NAME);
		} else if (flags & LS_FLAG_SORT_BY_SIZE) {
			bubble_sort(const_cast<directory_entry *>(result.entries), (result.result.number_entries), SORT_BY_SIZE);
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
				u32 max_name_length = get_max_name_length(result);

				// Minimum spaces between columns. Just hardcoded it and chose
				// because it looks reasonable.
				int minimum_padding = 6;

				// The width should be the maximum name length plus some padding.
				u32 NAME_COLUMN_WIDTH = max_name_length + minimum_padding;

				char type_char = (entry.type == fs_node_kind::directory) ? 'D' : 'F';

				console::get().writef("[%c] %s", type_char, entry.name);

				// Calculate padding dynamically based on the name length.
				u32 name_len = memops::strlen(entry.name);
				u32 padding = NAME_COLUMN_WIDTH - name_len;

				// Make sure there's at least one space of padding, regardless of the
				// name length.
				if (padding < 1) {
					padding = 1;
				}

				// Add the padding dynamically.
				for (u32 j = 0; j < padding; j++) {
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
};
} // namespace stacsos