/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#pragma once

#include <stacsos/kernel/mem/page-allocator.h>
#include <stacsos/memops.h>

namespace stacsos::kernel::mem {
class page_allocator_buddy : public page_allocator {
public:
	page_allocator_buddy(memory_manager &mm)
		: page_allocator(mm)
	{
		for (int i = 0; i <= LastOrder; i++) {
			free_list_[i] = nullptr;
		}

		// Initialize free list and cache
		// with null values and zero counts.
		for (int i = 0; i <= LastOrder; i++) {
			free_list_[i] = nullptr;

			// NEW: Initialize cache
			cache_count_[i] = 0;
			for (int j = 0; j < CacheSize; j++) {
				free_cache_[i][j] = nullptr;
			}
		}

		// Initialize all pending merges to zero.
		// In the constructor or init method
		for (int i = 0; i <= LastOrder; ++i) {
			for (int j = 0; j < (MAX_PENDING_MERGES / 64); ++j) {
				pending_merges_[i][j] = 0ULL;
			}
		}
	}

	virtual void insert_free_pages(page &range_start, u64 page_count) override;

	virtual page *allocate_pages(int order, page_allocation_flags flags = page_allocation_flags::none) override;
	virtual void free_pages(page &base, int order) override;

	virtual void dump() const override;

private:
	// Enable debug mode for additional logging
	// Used during the development stages for the 
	// additional logging of the
	// functions within the allocator.
	const bool DEBUG_MODE = true;
	static const int LastOrder = 16;

	page *free_list_[LastOrder + 1];
	u64 total_free_;

	// Cache size per order for freed blocks.
	// Chose the number 4 as a balance between memory usage and hit rate through
	// experimentation.
	static const int CacheSize = 4;

	// Simple cache to hold recently freed blocks for quick re-allocation.
	// Because we are not allowed to use any dynamic memory allocation,
	// we use a fixed-size cache per order.
	page *free_cache_[LastOrder + 1][CacheSize]; // 2D array: [order][cache_slot]
	int cache_count_[LastOrder + 1]; // Track how many blocks in each cache

	constexpr u64 pages_per_block(int order) const { return 1 << order; }

	constexpr bool block_aligned(int order, u64 pfn) { return !(pfn & (pages_per_block(order) - 1)); }

	// The amount of pages that can be pending merging at any one time.
	// This is used for the deferered merging optimization.
	static const size_t MAX_PENDING_MERGES = 64;

	// A 2D Array representing the pending merges for each order.
	// Each bit represents whether a merge is pending for a given block.
	// If the bit is set, then a merge is pending. If the bit is not set, then no merge is pending.
	// That means 1 u64 per order, and each u64 provides 64 bits,
	// so it can track up to 64 pending merges per order (one per bit).
	u64 pending_merges_[LastOrder + 1][MAX_PENDING_MERGES / 64];

	void insert_free_block(int order, page &block_start);
	void remove_free_block(int order, page &block_start);

	void split_block(int order, page &block_start);
	void merge_buddies(int order, page &buddy);
	u64 calculate_other_buddy_pfn(int order, u64 buddy_pfn);
	bool is_buddy_free(int order, u64 buddy_pfn);
	size_t pending_merge_index(u64 lower_pfn, int order);
	void set_pending_merge(u64 pfn, int order);
	bool is_pending_merge(u64 pfn, int order);
	void clear_pending_merge(u64 pfn, int order);
	int get_bit(int order, size_t idx) ;
	void cleanup_pending_merges();
	int find_in_cache(int order, u64 pfn);
	void remove_from_cache(int order, int index);
};
} // namespace stacsos::kernel::mem
