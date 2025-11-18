#pragma once
#include <stacsos/user-syscall.h>

namespace stacsos {
/**
 * Class which wraps over the native 'ls' syscall and provides utility functions
 * to print the results.
 */
class ls {
public: 
	static void ls_syscall_wrapper(const char *path, u8 flags);

	static void print_ls_result(const ls_result &result, u8 flags);
};
} // namespace stacsos