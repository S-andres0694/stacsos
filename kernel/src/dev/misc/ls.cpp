#include <stacsos/kernel/debug.h>
#include <stacsos/kernel/dev/misc/ls.h>
#include <stacsos/kernel/fs/file.h>
#include <stacsos/syscalls.h>

using namespace stacsos::kernel::dev;
using namespace stacsos::kernel::fs;
using namespace stacsos::kernel::dev::misc;
using namespace stacsos;

device_class ls::ls_device_class(device_class::root, "ls");

/**
 * The ls file class representing the ls device as a file.
 * This allows userspace programs to read the ls results directly from the device.
 * I implemented this in the form of a device that can be opened as a file, because I was having problems
 * with the user to kernel syscall interaction. Through a recommendation made by Dr. Spink, I decided to
 * follow this approach instead. Although it could have been possible to implement the ls syscall directly,
 * this approach allows for better modularity and separation of concerns, as the ls device handles the
 * computation and storage of ls results, while the file interface provides a standard way for userspace
 * programs to access those results.
 */

class ls_file : public file {
public:
	ls_file(ls &ls_device)
		: file((sizeof(final_product)))
		, ls_(ls_device)
	{
	}

	/**
	 * Reads the ls result from the device into the provided buffer.
	 * @param buffer The buffer to store the ls result.
	 * @param offset The offset to start reading from (not used in this implementation).
	 * @param length The length of the buffer.
	 * @return The number of bytes read into the buffer.
	 */

	virtual size_t pread(void *buffer, size_t offset, size_t length) override
	{
		if (length < sizeof(final_product)) {
			return 0;
		}

		stacsos::final_product *result = (stacsos::final_product *)buffer;

		result->result.code = this->ls_.prod.result.code;
		result->result.result_code = this->ls_.prod.result.result_code;
		result->result.number_entries = this->ls_.prod.result.number_entries;
		memops::memcpy(result->entries, this->ls_.prod.entries, sizeof(directory_entry) * this->ls_.prod.result.number_entries);

		return sizeof(final_product);
	}

	/**
	 * Writes to the ls device is not supported.
	 * @param buffer The buffer containing data to write (not used in this implementation).
	 * @param offset The offset to start writing to (not used in this implementation).
	 * @param length The length of the buffer (not used in this implementation).
	 * @return Always returns 0 as writing is not supported.
	 */

	virtual size_t pwrite(const void *buffer, size_t offset, size_t length) override { return 0; }

private:
	ls &ls_;
};

shared_ptr<file> ls::open_as_file() { return shared_ptr(new ls_file(*this)); }