#include <stacsos/kernel/debug.h>
#include <stacsos/kernel/dev/misc/ls-device.h>
#include <stacsos/kernel/fs/fat.h>

using namespace stacsos::kernel::dev;
using namespace stacsos::kernel::dev::misc;
using namespace stacsos::kernel::fs;

device_class ls_device::ls_device_class(ls::ls_device_class, "ls-device");

void ls_device::compute_ls(stacsos::kernel::fs::fat_node *node, u8 flags) { 
    this->result.code = syscall_result_code::ok;
    this->result.result_code = ls_result_code::ok;
    this->result.number_entries = node->children().count(); 
    for (const auto &child : node->children()) {
        stacsos::kernel::fs::fs_node_kind type = child->kind();
        if (type == stacsos::kernel::fs::fs_node_kind::directory) {
            dprintf("[DIR]  %s\n", child->name().c_str());
        } else {
            dprintf("[FILE] %s with size %llu\n", child->name().c_str(), child->data_size());
        }
    }
    dprintf("ls_device::compute_ls called\n");
    return;
}
