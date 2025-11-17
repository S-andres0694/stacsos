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
	static ls_result* ls_syscall_wrapper(char* path, u8 flags) {
		auto r = syscalls::ls_syscall(path, flags);
		return (ls_result *)r.data;
	}
};
} // namespace stacsos