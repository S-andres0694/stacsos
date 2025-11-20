#include <stacsos/kernel/dev/misc/ls-device.h>
#include <stacsos/kernel/debug.h>

using namespace stacsos::kernel::dev;
using namespace stacsos::kernel::dev::misc;

device_class ls_device::ls_device_class(ls::ls_device_class, "ls-device");

void ls_device::compute_ls(stacsos::kernel::fs::fat_node *node, u8 flags) { 
    this->result.code = syscall_result_code::not_supported;
    this->result.result_code = ls_result_code::ok;
    this->result.number_entries = 2;    
    dprintf("ls_device::compute_ls called\n");
    return;
}
