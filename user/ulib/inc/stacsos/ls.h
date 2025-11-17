#pragma once
#include <stacsos/user-syscall.h>
namespace stacsos {
class ls {
public:
    /**
     * Performs the 'ls' syscall to list the contents of a directory.
     * Acts as the bridge between user space and kernel space for the 'ls' operation.
     * @param path The path of the directory to list.
     * @param flags The flags to modify the behavior of the 'ls' command.
     * @return An ls_result structure containing the results of the 'ls' operation.
     */
	static ls_result* ls_syscall_wrapper(char* path, ls_flags flags) {
		ls_syscall_request request = { path, flags };
        // Prepare a result buffer to hold the results of the syscall
        ls_result result_buffer;
        // Perform the syscall and fill the result buffer with it.
        syscalls::ls_syscall(request, result_buffer);

        // Return a pointer to the result buffer containing the directory entries
		return &result_buffer;
	}
};
} // namespace stacsos