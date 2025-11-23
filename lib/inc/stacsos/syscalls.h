/* SPDX-License-Identifier: MIT */

/* StACSOS - Utility Library
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#pragma once
#include <stacsos/memops.h>

// I defined these by attempting to see how high I could go without causing
// page faults or other issues in QEMU. These are the maximum sizes I could arrive
// at that still worked reliably.
#define MAX_PATHNAME_LENGTH 128
#define MAX_RESULT_ENTRIES 50
// Example flag for long listing format. The bitfield would be 00000001
// Can extend this further by adding more #define statements with increasing bit values as the second number is the index of the bit to set.
#define LS_FLAG_LONG_LISTING (1 << 0)

namespace stacsos {
enum class syscall_result_code : u64 { ok = 0, not_found = 1, not_supported = 2 };

enum class syscall_numbers {
	exit = 0,
	open = 1,
	close = 2,
	read = 3,
	pread = 4,
	write = 5,
	pwrite = 6,
	set_fs = 7,
	set_gs = 8,
	alloc_mem = 9,
	start_process = 10,
	wait_for_process = 11,
	start_thread = 12,
	stop_current_thread = 13,
	join_thread = 14,
	sleep = 15,
	poweroff = 16,
	ioctl = 17,
	ls_syscall = 18
};

struct syscall_result {
	syscall_result_code code;
	u64 data;
} __packed;

// File system node kinds - copied from the kernel fs-node.h for consistency.
// I am probably able to include the kernel header here instead, but this is simpler for now.
enum class fs_node_kind { file, directory };

// Result codes for the 'ls' syscall
// If the syscall itself was successful, but the operation failed for some reason,
// I use these codes to indicate the specific failure reason so the console can handle it appropriately.
enum class ls_result_code { ok = 0, directory_does_not_exist = 1, file_was_passed = 2, unknown_error = 3, directory_empty = 4, unsupported_filesystem = 5 };

// 'ls' syscall request structure - used to pass parameters from user space to kernel space
typedef struct ls_syscall_request {
	// The path to list
	char *path;

	// Flags for the ls command
	u8 flags;
} ls_syscall_request;

// Directory entry structure - shared between kernel and user space
typedef struct directory_entry {
	// The name of the file/directory that is being represented
	char name[MAX_PATHNAME_LENGTH];

	// The type of this entry (file/directory).
	fs_node_kind type;

	// The size of this entry (in bytes).
	u64 size;
} directory_entry;

// Result structure for the 'ls' syscall. This is going to be used by the
// user space application to interpret the results of the syscall.
typedef struct ls_result {
	// The result code of the syscall
	syscall_result_code code;

	// The result code of the syscall
	// To allow for a more detailed result within the ls operation itself.
	// I wanted to have a separate result code here.
	ls_result_code result_code;

	// The number of entries returned
	u64 number_entries;

} ls_result;

/**
 * Final product structure for the 'ls' syscall. This contains the result structure
 * and an array of directory entries.
 * For some reason adding the array onto the ls_result struct itself caused issues with
 * page faults and even QEMU Crashes, so I've created this separate struct to hold both the result
 * and the entries.
 */

struct final_product {
	ls_result result;
	directory_entry entries[MAX_RESULT_ENTRIES];

	final_product &operator=(const final_product &other)
	{
		if (this != &other) {
			this->result = other.result;
			memops::memcpy(this->entries, other.entries, sizeof(entries));
		}
		return *this;
	}

	final_product() = default;

	final_product(const final_product &other)
	{
		this->result = other.result;
		memops::memcpy(this->entries, other.entries, sizeof(entries));
	}
};
} // namespace stacsos
