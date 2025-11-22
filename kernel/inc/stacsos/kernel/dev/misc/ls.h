#pragma once

#include <stacsos/kernel/dev/device.h>
#include <stacsos/kernel/fs/fat.h>
#include <stacsos/syscalls.h>
#include <stacsos/ls-cache.h>

namespace stacsos::kernel::dev::misc {
class ls : public device {
public:
	static device_class ls_device_class;

	final_product prod;

	ls_cache cache_;

	ls(device_class &dc, bus &owner)
		: device(dc, owner)
	{
	}

	virtual void compute_ls(const char *path, u8 flags) { return; }

	virtual shared_ptr<fs::file> open_as_file() override;
};
}