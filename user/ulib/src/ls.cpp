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

void ls::ls_syscall_wrapper(const char *path)
{
	syscalls::ls_syscall(path);
}