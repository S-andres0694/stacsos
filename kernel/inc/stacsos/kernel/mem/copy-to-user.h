#pragma once
#include <stacsos/memops.h>

/**
 * This function attempts to copy a block of memory from a kernel buffer to a
 * user-provided memory location. It checks that the destination address resides
 * in the userspace address range (i.e., is below the kernel virtual memory start).
 *
 * @param user_buffer  Pointer to the destination buffer in user space.
 *                     This must be a valid (writable) userspace address.
 * @param kernel_buffer Pointer to the source buffer in kernel space.
 *                      This must be a valid kernel address and should remain readable during the copy.
 * @param len           Number of bytes to copy from the kernel buffer to the user buffer.
 *                      Must not wrap around the userspace boundary.
 * @return true if the copy was performed (destination address is userspace), false if not.
 */

static inline bool copy_to_user(void *user_buffer, const void *kernel_buffer, size_t len)
{
	uintptr_t uptr = (uintptr_t)user_buffer;
	// Addresses at or above 0xFFFFFFFF80000000 are kernel space
	// I figured this by fuzzy searching though the repo for "KERNEL" to try
	// and locate the kernel space address range. I found that in the kernel/stacsos.ld
	// file, this was commented as the start of the kernel space.
	if (uptr >= 0xFFFFFFFF80000000ULL) {
		return false;
	}

	// Learned through stack overflow that memcpy is a bad choice for this task: 
    // https:// stackoverflow.com/questions/40415046/can-i-say-copy-to-user-copy-from-user-is-a-memcpy-with-access-ok
	stacsos::memops::memcpy(user_buffer, kernel_buffer, len);
	return true;
}