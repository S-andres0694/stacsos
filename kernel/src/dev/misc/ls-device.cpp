#include <stacsos/kernel/debug.h>
#include <stacsos/kernel/dev/misc/ls-device.h>
#include <stacsos/kernel/fs/fat.h>
#include <stacsos/console.h>

using namespace stacsos::kernel::dev;
using namespace stacsos::kernel::dev::misc;
using namespace stacsos::kernel::fs;

device_class ls_device::ls_device_class(ls::ls_device_class, "ls-device");

void ls_device::compute_ls(const char* path, u8 flags) {
    // Clear the previous result
    memops::memset(&this->result, 0, sizeof(ls_result));

	// Look for the directory node.
	// I know from the kernel/src/main.cpp file that all
	// files mounted under the root are part of a FAT filesystem.
	// So we can just use the VFS to look up the path.
	// And we know it will be FAT specific nodes.
	auto *node = vfs::get().lookup(path);
	if (node == nullptr) {
		this->result.code = syscall_result_code::ok;
		this->result.result_code = ls_result_code::directory_does_not_exist;
		this->result.number_entries = 0;
		dprintf("Directory does not exist: %s\n", path);
		return;
	}

	// Ensure it's a directory
	stacsos::kernel::fs::fs_node_kind kind = node->kind();
	if (kind != stacsos::kernel::fs::fs_node_kind::directory) {
		this->result.code = syscall_result_code::ok;
		this->result.result_code = ls_result_code::file_was_passed;
		this->result.number_entries = 0;
		dprintf("Path is not a directory: %s\n", path);
		return;
	}

	// Ensure that the node is a FAT directory node
	fs_type_hint type = node->fs().type_hint();
	if (type != fs_type_hint::fat) {
		this->result.code = syscall_result_code::ok;
		this->result.result_code = ls_result_code::unsupported_filesystem;
		this->result.number_entries = 0;
		dprintf("Unsupported filesystem for ls: %s\n", path);
		return;
	} else {
		dprintf("FAT filesystem detected for path: %s\n", path);
	}

	// It's a FAT directory, so we can static cast to a fat_node
	fat_node &fat_dir_node = static_cast<fat_node &>(*node);
	if (fat_dir_node.children().count() == 0) {
		this->result.code = syscall_result_code::ok;
		this->result.result_code = ls_result_code::directory_empty;
		this->result.number_entries = 0;
		dprintf("Directory is empty: %s\n", path);
		return;
	}

	this->result.code = syscall_result_code::ok;
    this->result.result_code = ls_result_code::ok;
    this->result.number_entries = fat_dir_node.children().count(); 
	for (const auto &child : fat_dir_node.children()) {
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
