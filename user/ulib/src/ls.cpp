#include <stacsos/ls.h>
#include <stacsos/user-syscall.h>
#include <stacsos/console.h>

using namespace stacsos;

/**
 * Performs the 'ls' syscall to list the contents of a directory.
 * Acts as the bridge between user space and kernel space for the 'ls' operation.
 * @param path The path of the directory to list.
 * @param flags The flags to modify the behavior of the 'ls' command.
 * @return An ls_result structure containing the results of the 'ls' operation.
 */

void ls::ls_syscall_wrapper(const char *path, u8 flags)
{
	syscalls::ls_syscall(path, flags);
}

/**
 * Creates a new ls_result structure initialized to default values.
 * @return A new ls_result structure with default values.
 */

ls_result ls::new_ls_result()
{
	ls_result result;
	result.code = syscall_result_code::ok;
	result.result_code = ls_result_code::ok;
	result.number_entries = 0;
	return result;
}
