#include <stacsos/kernel/debug.h>
#include <stacsos/kernel/dev/misc/ls-device.h>
#include <stacsos/kernel/fs/fat.h>
#include <stacsos/kernel/fs/vfs.h>

using namespace stacsos::kernel::dev;
using namespace stacsos::kernel::dev::misc;
using namespace stacsos::kernel::fs;

device_class ls_device::ls_device_class(ls::ls_device_class, "ls-device");

void ls_device::compute_ls(const char *path, u8 flags)
{
	if (stacsos::memops::strcmp(path, this->last_lookup_path) == 0) {
		dprintf("Path matches last lookup path: %s\n", path);
		return;
	}

	// Clear the previous result since we're computing a new one
	memops::memset(&this->prod.result, 0, sizeof(final_product));

	// Look for the directory node.
	// I know from the kernel/src/main.cpp file that all
	// files mounted under the root are part of a FAT filesystem.
	// So we can just use the VFS to look up the path.
	// And we know it will be FAT specific nodes.
	auto *node = vfs::get().lookup(path);
	if (node == nullptr) {
		this->prod.result.code = syscall_result_code::ok;
		this->prod.result.result_code = ls_result_code::directory_does_not_exist;
		this->prod.result.number_entries = 0;
		dprintf("Directory does not exist: %s\n", path);
		return;
	}

	final_product cache_ent;
	// Check the cache first
	stacsos::string path_str(path);
	if (this->cache_.lookup(path_str, cache_ent)) {
		// Make sure the cache entry is not dirty
		if (node->dirty_cache_bit) {
			dprintf("Cache entry is dirty for path: %s\n", path);
		} else {
			dprintf("Cache hit for path: %s\n", path);
			this->prod = cache_ent;
			memops::strncpy(this->last_lookup_path, path, sizeof(this->last_lookup_path) - 1);
			this->last_lookup_path[sizeof(this->last_lookup_path) - 1] = '\0';
			return;
		}
	} else {
		dprintf("Cache miss for path: %s\n", path);
	}

	// Ensure it's a directory
	stacsos::kernel::fs::fs_node_kind kind = node->kind();
	if (kind != stacsos::kernel::fs::fs_node_kind::directory) {
		this->prod.result.code = syscall_result_code::ok;
		this->prod.result.result_code = ls_result_code::file_was_passed;
		this->prod.result.number_entries = 0;
		dprintf("Path is not a directory: %s\n", path);
		return;
	}

	// Ensure that the node is a FAT directory node
	fs_type_hint type = node->fs().type_hint();
	if (type != fs_type_hint::fat) {
		this->prod.result.code = syscall_result_code::ok;
		this->prod.result.result_code = ls_result_code::unsupported_filesystem;
		this->prod.result.number_entries = 0;
		dprintf("Unsupported filesystem for ls: %s\n", path);
		return;
	} else {
		dprintf("FAT filesystem detected for path: %s\n", path);
	}

	// It's a FAT directory, so we can static cast to a fat_node
	fat_node &fat_dir_node = static_cast<fat_node &>(*node);
	if (fat_dir_node.children().count() == 0) {
		this->prod.result.code = syscall_result_code::ok;
		this->prod.result.result_code = ls_result_code::directory_empty;
		this->prod.result.number_entries = 0;
		dprintf("Directory is empty: %s\n", path);
		return;
	}

	for (const auto &child : fat_dir_node.children()) {
		u64 idx = this->prod.result.number_entries;

		dprintf("Processing child node: %s\n", child->name().c_str());

		// Prevent overflow of the entries buffer
		if (idx >= MAX_RESULT_ENTRIES) {
			dprintf("[WARN] Too many directory entries — skipping '%s'\n", child->name().c_str());
			break;
		}
		directory_entry &entry = this->prod.entries[idx];
		// Copy name safely
		memops::strncpy(entry.name, child->name().c_str(), sizeof(entry.name) - 1);
		entry.name[sizeof(entry.name) - 1] = '\0';
		// Determine type
		stacsos::kernel::fs::fs_node_kind type = child->kind();
		if (type == stacsos::kernel::fs::fs_node_kind::directory) {
			entry.type = fs_node_kind::directory;
			entry.size = 0;
		} else {
			entry.type = fs_node_kind::file;
			entry.size = child->data_size();
		}
		// Only increment after successfully filling the entry
		this->prod.result.number_entries++;
	}

	// Add the result to the cache
	final_product new_cache_entry = this->prod;
	this->cache_.put(path_str, new_cache_entry);

	// Update the last lookup path
	memops::strncpy(this->last_lookup_path, path, sizeof(this->last_lookup_path) - 1);
	this->last_lookup_path[sizeof(this->last_lookup_path) - 1] = '\0';
	dprintf("Last lookup path updated to: %s\n", this->last_lookup_path);

	// Mark the cache as clean
	node->dirty_cache_bit = false;

	dprintf("Cached ls result for path: %s\n", path);
	return;
}
