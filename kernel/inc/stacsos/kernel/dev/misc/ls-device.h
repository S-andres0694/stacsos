#pragma once

#include <stacsos/kernel/dev/misc/ls.h>
#include <stacsos/syscalls.h>

namespace stacsos::kernel::dev::misc {
    class ls_device: public ls {
    public:
        static device_class ls_device_class;

        ls_device(bus &owner)
            : ls(ls_device_class, owner)
        {
			result.code = syscall_result_code::ok;
			result.result_code = ls_result_code::ok;
			result.number_entries = 0;
		}
        
        virtual void configure() override { }

        virtual void compute_ls(stacsos::kernel::fs::fat_node *node, u8 flags) override;

	};
} // namespace stacsos::kernel::dev::misc