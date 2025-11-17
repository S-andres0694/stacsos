/* SPDX-License-Identifier: MIT */

/* StACSOS - userspace standard library
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#pragma once

#include <stacsos/syscalls.h>
#define MAX_PATHNAME_LENGTH 256
#define MAX_RESULT_ENTRIES 256

namespace stacsos {

// File system node kinds - copied from the kernel fs-node.h for consistency.
// I am probably able to include the kernel header here instead, but this is simpler for now.	
enum class fs_node_kind { file, directory };

// Result codes for the 'ls' syscall
// If the syscall itself was successful, but the operation failed for some reason,
// I use these codes to indicate the specific failure reason so the console can handle it appropriately.
enum class ls_result_code { ok = 0, directory_does_not_exist = 1, file_was_passed = 2, unknown_error = 3 };

// Flags for the 'ls' syscall
typedef struct ls_flags {
	// Long listing flag - if set, 'ls' should provide detailed information about each entry. Detailed by /usr/ls -l
	bool long_listing_flag;
} ls_flags;


// 'ls' syscall request structure - used to pass parameters from user space to kernel space
typedef struct ls_syscall_request {
	// The path to list
	char *path;

	// Flags for the ls command
	ls_flags flags;
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

	// The list of entries returned following the custom type definition.
	// Hard limit of 256 entries for simplicity and might be changed.
	directory_entry entries[MAX_RESULT_ENTRIES];
} ls_result;

struct rw_result {
	syscall_result_code code;
	u64 length;
};

struct fa_result {
	syscall_result_code code;
	u64 id;
};

struct alloc_result {
	syscall_result_code code;
	void *ptr;
};

class syscalls {
public:
	static syscall_result_code exit(u64 result) { return syscall1(syscall_numbers::exit, result).code; }

	static syscall_result_code set_fs(u64 value) { return syscall1(syscall_numbers::set_fs, value).code; }
	static syscall_result_code set_gs(u64 value) { return syscall1(syscall_numbers::set_gs, value).code; }

	static fa_result open(const char *path)
	{
		auto r = syscall1(syscall_numbers::open, (u64)path);
		return fa_result { r.code, r.data };
	}

	static syscall_result_code close(u64 id) { return syscall1(syscall_numbers::close, id).code; }

	static rw_result read(u64 object, void *buffer, u64 length)
	{
		auto r = syscall3(syscall_numbers::read, object, (u64)buffer, length);
		return rw_result { r.code, r.data };
	}

	static rw_result write(u64 object, const void *buffer, u64 length)
	{
		auto r = syscall3(syscall_numbers::write, object, (u64)buffer, length);
		return rw_result { r.code, r.data };
	}

	static rw_result pwrite(u64 object, const void *buffer, u64 length, size_t offset)
	{
		auto r = syscall4(syscall_numbers::pwrite, object, (u64)buffer, length, offset);
		return rw_result { r.code, r.data };
	}

	static rw_result pread(u64 object, void *buffer, u64 length, size_t offset)
	{
		auto r = syscall4(syscall_numbers::pread, object, (u64)buffer, length, offset);
		return rw_result { r.code, r.data };
	}

	static rw_result ioctl(u64 object, u64 cmd, void *buffer, u64 length)
	{
		auto r = syscall4(syscall_numbers::ioctl, object, cmd, (u64)buffer, length);
		return rw_result { r.code, r.data };
	}

	static alloc_result alloc_mem(u64 size)
	{
		auto r = syscall1(syscall_numbers::alloc_mem, size);
		return alloc_result { r.code, (void *)r.data };
	}

	static syscall_result start_process(const char *path, const char *args) { return syscall2(syscall_numbers::start_process, (u64)path, (u64)args); }
	static syscall_result wait_process(u64 id) { return syscall1(syscall_numbers::wait_for_process, id); }

	static syscall_result start_thread(void *entrypoint, void *arg) { return syscall2(syscall_numbers::start_thread, (u64)entrypoint, (u64)arg); }
	static syscall_result join_thread(u64 id) { return syscall1(syscall_numbers::join_thread, id); }
	static syscall_result stop_current_thread() { return syscall0(syscall_numbers::stop_current_thread); }

	static syscall_result sleep(u64 ms) { return syscall1(syscall_numbers::sleep, ms); }

	static void poweroff() { syscall0(syscall_numbers::poweroff); }

	/**
	 * Performs the 'ls' syscall to list the contents of a directory.
	 * Fills in the provided result buffer with the results of the syscall.
	 * @param request The ls_syscall_request structure containing the flags and path for the syscall.
	 * @param result_buffer The ls_result structure to be filled with the results of the syscall
	 */
	static void ls_syscall(const ls_syscall_request &request, ls_result &result_buffer)
	{
		// This syscall does not return a value, instead it fills in the result buffer provided.
		syscall2(syscall_numbers::ls_syscall, (u64)&request, (u64)&result_buffer);
	}

private:
	static syscall_result syscall0(syscall_numbers id)
	{
		syscall_result r;
		asm volatile("syscall" : "=a"(r.code), "=d"(r.data) : "a"(id) : "flags", "rcx");
		return r;
	}

	static syscall_result syscall1(syscall_numbers id, u64 arg0)
	{
		syscall_result r;
		asm volatile("syscall" : "=a"(r.code), "=d"(r.data) : "a"(id), "D"(arg0) : "flags", "rcx");
		return r;
	}

	static syscall_result syscall2(syscall_numbers id, u64 arg0, u64 arg1)
	{
		syscall_result r;
		asm volatile("syscall" : "=a"(r.code), "=d"(r.data) : "a"(id), "D"(arg0), "S"(arg1) : "flags", "rcx");
		return r;
	}

	static syscall_result syscall3(syscall_numbers id, u64 arg0, u64 arg1, u64 arg2)
	{
		syscall_result r;
		asm volatile("syscall" : "=a"(r.code), "=d"(r.data) : "a"(id), "D"(arg0), "S"(arg1), "d"(arg2) : "flags", "rcx");
		return r;
	}

	static syscall_result syscall4(syscall_numbers id, u64 arg0, u64 arg1, u64 arg2, u64 arg3)
	{
		register u64 forced_arg3 asm("r8") = arg3;

		syscall_result r;
		asm volatile("syscall" : "=a"(r.code), "=d"(r.data) : "a"(id), "D"(arg0), "S"(arg1), "d"(arg2), "r"(forced_arg3) : "flags", "rcx", "r11");
		return r;
	}
};
} // namespace stacsos
