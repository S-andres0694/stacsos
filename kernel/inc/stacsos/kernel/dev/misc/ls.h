#pragma once

#include <stacsos/kernel/dev/device.h>
#include <stacsos/kernel/fs/fat.h>
#include <stacsos/ls-cache.h>
#include <stacsos/syscalls.h>

/**
 * Defines a generic ls device that can be extended for specific implementations.
 * Normally, there would be a single LS device per OS, but this design allows for
 * multiple LS devices if needed, which could perhaps come as use for different file systems.
 */

namespace stacsos::kernel::dev::misc {
class ls : public device {
public:
	/**
	 * The device class for the ls device.
	 * All instances of ls share the same device class.
	 */
	static device_class ls_device_class;

	/**
	 * The internal buffer storing the result of the last ls computation.
	 * It also works as a sort of cache to avoid recomputing ls results unnecessarily
	 * when looking up the same directory multiple consecutive times.
	 */
	final_product prod;

	/**
	 * The ls cache for the device.
	 * Implemented as an LRU cache to store recently accessed directory listings. It has
	 * a capacity of 8 entries, allowing for a good balance between memory usage and cache hit rate. It will also free the oldest entry when full, making sure
	 * that the memory footprint remains relatively constant (varies slightly given the dynamic nature of the entries).
	 */
	ls_cache cache_;

	/**
	 * The path of the last lookup performed by the device.
	 * Used to determine if a new lookup is necessary or if the cached result can be reused.
	 * Although it is unlikely that users will perform consecutive reads of the same unchanged directory, this solution can provide an O(1) check to avoid
	 * unnecessary computations in such cases.
	 *
	 */
	char last_lookup_path[MAX_PATHNAME_LENGTH];

	/**
	 * Constructor for the ls device.
	 * @param dc The device class for the ls device.
	 * @param owner The bus that owns this device.
	 */

	ls(device_class &dc, bus &owner)
		: device(dc, owner)
	{
	}

	/**
	 * Computes the ls result for the given path and flags.
	 * This is a pure virtual function that must be implemented by derived classes
	 * as the actual ls computation may vary depending on the file system or other factors.
	 * @param path The directory path to list.
	 * @param flags The flags modifying the ls behavior.
	 */

	virtual void compute_ls(const char *path, u8 flags) { return; }

	/**
	 * Opens the ls device as a file.
	 * This allows userspace programs to read the ls results directly from the device
	 * by using the pread method.
	 */

	virtual shared_ptr<fs::file> open_as_file() override;
};
} // namespace stacsos::kernel::dev::misc