#pragma once

#include <stacsos/kernel/dev/device.h>
#include <stacsos/kernel/fs/fat.h>
#include <stacsos/syscalls.h>

namespace stacsos::kernel::dev::misc {
class ls : public device {
public:
	static device_class ls_device_class;

	ls_result result;

	ls(device_class &dc, bus &owner)
		: device(dc, owner)
	{
	}

	virtual void compute_ls(stacsos::kernel::fs::fat_node *node, u8 flags);

	virtual shared_ptr<fs::file> open_as_file() override;
};
}