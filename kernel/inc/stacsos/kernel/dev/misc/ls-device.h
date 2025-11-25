#pragma once

#include <stacsos/kernel/dev/misc/ls.h>
#include <stacsos/syscalls.h>

namespace stacsos::kernel::dev::misc {
class ls_device : public ls {
public:
	static device_class ls_device_class;

	ls_device(bus &owner)
		: ls(ls_device_class, owner)
	{
	}

	virtual void configure() override { }

	virtual void compute_ls(const char *path, u8 flags) override;
};
} // namespace stacsos::kernel::dev::misc