#include <stacsos/kernel/dev/misc/ls-device.h>

using namespace stacsos::kernel::dev;
using namespace stacsos::kernel::dev::misc;

device_class ls_device::ls_device_class(ls::ls_device_class, "ls-device");

void ls_device::compute_ls(stacsos::kernel::fs::fat_node *node, u8 flags) { return; }
