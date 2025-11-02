/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024, 2025
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <stacsos/kernel/debug.h>
#include <stacsos/kernel/mem/page-allocator-buddy.h>
#include <stacsos/kernel/mem/page.h>
#include <stacsos/memops.h>

using namespace stacsos;
using namespace stacsos::kernel;
using namespace stacsos::kernel::mem;

// Represents the contents of a free page, that can hold useful metadata.
struct page_metadata {
	page *next_free;
};

/**
 * @brief Returns a pointer to the metadata structure that is held within a free page.  This CANNOT be used on
 * pages that have been allocated, as they are owned by the requesting code.  Once pages have been freed, or
 * are being returned to the allocator, this metadata can be used.
 *
 * @param page The page on which to retrieve the metadata struct.
 * @return page_metadata* The metadata structure.
 */
static inline page_metadata *metadata(page *page) { return (page_metadata *)page->base_address_ptr(); }

/**
 * @brief Dumps out (via the debugging routines) the current state of the buddy page allocator's free lists
 */
void page_allocator_buddy::dump() const
{
	// Print out a header, so we can quickly identify this output in the debug stream.
	dprintf("*** buddy page allocator - free list ***\n");

	// Loop over each order that our allocator is responsible for, from zero up to *and
	// including* LastOrder.
	for (int order = 0; order <= LastOrder; order++) {
		// Print out the order number (with a leading zero, so that it's nicely aligned)
		dprintf("[%02u] ", order);

		// Get the pointer to the first free page in the free list.
		page *current_free_page = free_list_[order];

		// While there /is/ currently a free page in the list...
		while (current_free_page) {
			// Print out the extents of this page, i.e. its base address (at byte granularity), up to and including the last
			// valid address.  Remember: these are PHYSICAL addresses.
			dprintf("%lx--%lx ", current_free_page->base_address(), (current_free_page->base_address() + ((1 << order) << PAGE_BITS)) - 1);

			// Advance to the next page, by interpreting the free page as holding metadata, and reading
			// the appropriate field.
			current_free_page = metadata(current_free_page)->next_free;
		}

		// New line for the next order.
		dprintf("\n");
	}
}

/**
 * @brief Inserts pages that are known to be free into the buddy allocator.
 *
 * ** You are required to implement this function **
 *
 * @param range_start The first page in the range.
 * @param page_count The number of pages in the range.
 */
void page_allocator_buddy::insert_free_pages(page &range_start, u64 page_count)
{
	// Ensure that we have at least one page to insert because otherwise, there's nothing to do.
	assert(page_count > 0);

	// Start with the first page in the range.
	page *current_page = &range_start;

	// Also make sure to keep track of the number of remaining pages to insert,
	// as well as the number of pages that we have successfully inserted.
	u64 remaining = page_count;
	u64 inserted = 0;

	// While there are remaining pages to insert I attempt to
	while (remaining > 0) {
		// Find the largest possible order that can be inserted
		// at the current page, given the alignment and remaining pages.
		int order = LastOrder;
		while (order > 0 && (!block_aligned(order, current_page->pfn()) || pages_per_block(order) > remaining)) {
			--order;
		}

		// free_pages should handle the insertion and coalescing
		// of the block into the buddy allocator.
		free_pages(*current_page, order);

		// Move the current page forward by the size of the block we just inserted.
		u64 size_of_the_block = pages_per_block(order);
		current_page = &page::get_from_pfn(current_page->pfn() + size_of_the_block);

		// Make all of the necessary updates to the remaining and inserted counters.
		remaining -= size_of_the_block;
		inserted += size_of_the_block;
	}

	// Update the total free pages count.
	total_free_ += inserted;
}

/**
 * @brief Inserts a block of pages into the free list for the given order.
 *
 * @param order The order in which to insert the free blocks.
 * @param block_start The starting page of the block to be inserted.
 */
void page_allocator_buddy::insert_free_block(int order, page &block_start)
{
	// Assert that the given order is in the range of orders we support.
	assert(order >= 0 && order <= LastOrder);

	// Assert that the starting page in the block is aligned to the requested order.
	assert(block_aligned(order, block_start.pfn()));

	// Iterate through the free list, until we get to the position where the
	// block should be inserted, i.e. ordered by page base address.
	// The comparison in the while loop is valid, because page descriptors (which we
	// are dealing with) are contiguous in memory -- just like the pages they represent.
	page *target = &block_start;
	page **slot = &free_list_[order];
	while (*slot && *slot < target) {
		slot = &(metadata(*slot)->next_free);
	}

	// Make sure the block wasn't already in the free list.
	assert(*slot != target);

	// Update the target block (i.e. the block we're inserting) to point to
	// the candidate slot -- which is the next pointer.  Then, update the
	// candidate slot to point to this block; thus inserting the block into the
	// linked-list.
	metadata(target)->next_free = *slot;
	*slot = target;
}

/**
 * @brief Removes a block of pages from the free list of the specified order.
 *
 * @param order The order in which to remove a free block.
 * @param block_start The starting page of the block to be removed.
 */
void page_allocator_buddy::remove_free_block(int order, page &block_start)
{
	// Assert that the given order is in the range of orders we support.
	assert(order >= 0 && order <= LastOrder);

	// Assert that the starting page in the block is aligned to the requested order.
	assert(block_aligned(order, block_start.pfn()));

	// Loop through the free list for the given order, until we find the
	// block to remove.
	page *target = &block_start;
	page **candidate_slot = &free_list_[order];
	while (*candidate_slot && *candidate_slot != target) {
		candidate_slot = &(metadata(*candidate_slot)->next_free);
	}

	// Assert that the candidate block actually exists, i.e. the requested
	// block really was in the free list for the order.
	assert(*candidate_slot == target);

	// The candidate slot is the "next" pointer of the target's previous block.
	// So, update that to point that to the target block's next pointer, thus
	// removing the requested block from the linked-list.
	*candidate_slot = metadata(target)->next_free;

	// Clear the next_free pointer of the target block.
	metadata(target)->next_free = nullptr;
}

/**
 * @brief Splits a free block of pages from a given order, into two halves into a lower order.
 *
 * ** You are required to implement this function **
 * @param order The order in which the free block current exists.
 * @param block_start The starting page of the block to be split.
 */
void page_allocator_buddy::split_block(int order, page &block_start)
{
	// Function cannot be called with an invalid order.
	assert(0 < order && order <= LastOrder);

	// We know that this function can only be called internally
	// when there is a free block available to be split as the
	// as the function is private.
	// Therefore, there's no need for an additional check of
	// freedom for the block being split.

	// Make sure that the block to be split is aligned to
	// the passed order.
	assert(block_aligned(order, block_start.pfn()));

	// Calculate the PFNs of the two resulting buddy blocks
	// after the split.
	u64 first_buddy_pfn = block_start.pfn();

	// Since the block has exactly 2^k pages, the second buddy
	// starts exactly halfway through the block.
	u64 second_buddy_pfn = block_start.pfn() + (1 << (order - 1));

	// Get the starting page references of each buddy block.
	page &first_buddy = page::get_from_pfn(first_buddy_pfn);
	page &second_buddy = page::get_from_pfn(second_buddy_pfn);

	// Remove the original block from the free list
	remove_free_block(order, block_start);

	// Insert the two smaller blocks into the next lower order,
	// completing the split.
	insert_free_block(order - 1, first_buddy);
	insert_free_block(order - 1, second_buddy);
}

/**
 * @brief Merges two buddy-adjacent free blocks in one order, into a block in the next higher order.
 *
 * ** You are required to implement this function **
 * @param order The order in which to merge buddies.
 * @param buddy Either buddy page in the free block.
 */
void page_allocator_buddy::merge_buddies(int order, page &buddy)
{
	// Function cannot be called with an invalid order.
	assert(0 <= order && order <= LastOrder);

	// We know that the page that was given has
	// to be free if is going to be merged
	// as said by the spec (Page 4).
	// and since it is an internal function
	// we can skip the freedom check for the passed
	// page.

	// Get the buddy candidate.
	page &other_buddy = page::get_from_pfn(calculate_other_buddy_pfn(order, buddy.pfn()));

	// They must both be aligned to the passed order.
	assert(block_aligned(order, buddy.pfn()) && block_aligned(order, other_buddy.pfn()));

	// Make sure that the buddy being merged is actually free.
	assert(is_buddy_free(order, other_buddy.pfn()));

	// Because they are going to be merged, they can no longer be free.
	remove_free_block(order, buddy);
	remove_free_block(order, other_buddy);

	// Create the newly merged block by choosing the starting page
	// based on the smallest PFN.
	u64 merged_pfn = (buddy.pfn() < other_buddy.pfn()) ? buddy.pfn() : other_buddy.pfn();
	page &merged_block = page::get_from_pfn(merged_pfn);

	// Insert the merged block into the next higher order
	insert_free_block(order + 1, merged_block);
}

/**
 * @brief Allocates pages, using the buddy algorithm.
 *
 * ** You are required to implement this function **
 * @param order The order of pages to allocate (i.e. 2^order number of pages)
 * @param flags Any allocation flags to take into account.
 * @return page* The starting page of the block that was allocated, or nullptr if the allocation cannot be satisfied.
 */
page *page_allocator_buddy::allocate_pages(int order, page_allocation_flags flags)
{
	// Ensure that the passed order is valid.
	assert(order >= 0 && order <= LastOrder);

	for (int current_order = order; current_order <= LastOrder; ++current_order) {
		if (free_list_[current_order] != nullptr) {
			// We find a suitable block at the current order.
			page *block_of_pages = free_list_[current_order];

			// If the current order is larger than the requested order,
			// we need to split the block down to the requested order.
			while (current_order > order) {
				split_block(current_order, *block_of_pages);
				--current_order;
			}

			// We now have a block at the requested order.
			remove_free_block(order, *block_of_pages);

			// Update the total free pages count.
			total_free_ -= pages_per_block(order);

			// If the zero flag is set, zero out the allocated pages.
			if ((flags & page_allocation_flags::zero) == page_allocation_flags::zero) {
				memops::pzero(block_of_pages->base_address_ptr(), pages_per_block(order));
			}

			return block_of_pages;
		}
	}

	// If no suitable block was found, trigger cleanup of pending merges and retry
	cleanup_pending_merges();

	// Retry the same allocation process after cleanup
	for (int current_order = order; current_order <= LastOrder; ++current_order) {
		if (free_list_[current_order] != nullptr) {
			page *allocated_page = free_list_[current_order];
			remove_free_block(current_order, *allocated_page);
			while (current_order > order) {
				--current_order;
				split_block(current_order + 1, *allocated_page);
			}

			total_free_ -= pages_per_block(order);

			if ((flags & page_allocation_flags::zero) == page_allocation_flags::zero) {
				memops::pzero(allocated_page->base_address_ptr(), pages_per_block(order));
			}
			return allocated_page;
		}
	}

	// If still no block, print a message as the allocation has failed.
	dprintf("Buddy allocator: Unable to satisfy page allocation request\n");
	return nullptr;
}

/**
 * @brief Frees previously allocated pages, using the buddy algorithm.
 *
 * ** You are required to implement this function **
 * @param block_start The starting page of the block to be freed.
 * @param order The order of the block being freed.
 */
void page_allocator_buddy::free_pages(page &block_start, int order)
{
	assert(order >= 0 && order <= LastOrder);
	assert(block_aligned(order, block_start.pfn()));

	page *current_block = &block_start;

	// Insert the block into the free list first
	insert_free_block(order, *current_block);

	if (order < LastOrder) {
		u64 buddy_pfn = calculate_other_buddy_pfn(order, current_block->pfn());
		page &buddy = page::get_from_pfn(buddy_pfn);
		if (block_aligned(order, buddy.pfn()) && is_buddy_free(order, buddy.pfn())) {
			u64 lower_pfn = (block_start.pfn() < buddy_pfn) ? block_start.pfn() : buddy_pfn;
			if (is_pending_merge(lower_pfn, order)) {
				// If a merge is already pending for this block, we perform the merge now.
				clear_pending_merge(lower_pfn, order);
				merge_buddies(order, *current_block);

				// After merging, we need to update the current block to the newly merged block.
				u64 merged_pfn = (block_start.pfn() < buddy_pfn) ? block_start.pfn() : buddy_pfn;
				current_block = &page::get_from_pfn(merged_pfn);

				// Recursively attempt to merge at the next higher order.
				free_pages(*current_block, order + 1);
			} else {
				// Mark this block as pending merge for future merges.
				set_pending_merge(lower_pfn, order);
			}
		}
	}

	// Update the total free pages count.
	total_free_ += pages_per_block(order);
	return;
}

/**
 * @brief Calculates the PFN of the other buddy in the same order.
 *
 * @param order The order of the block.
 * @param buddy_pfn The PFN of the given buddy block.
 * @return The PFN of the other buddy block.
 */
u64 page_allocator_buddy::calculate_other_buddy_pfn(int order, u64 buddy_pfn) { return buddy_pfn ^ (1 << order); }

/**
 * @brief Checks if the buddy block is free.
 *
 * @param order The order of the block.
 * @param buddy_pfn The PFN of the given buddy block.
 * @return true if the buddy is free, false otherwise.
 */
bool page_allocator_buddy::is_buddy_free(int order, u64 buddy_pfn)
{
	// Iterates through the free list of the given order
	// to check if the buddy with the given PFN is free.
	page *current = free_list_[order];
	while (current) {
		if (current->pfn() == buddy_pfn)
			return true;
		current = metadata(current)->next_free;
	}
	return false;
}

/**
 * @brief Calculates the index in the pending merges array for a given PFN and order.
 * This index is calculated using a simple hash function to distribute all of the PFNs across the available slots.
 *
 * @param lower_pfn The lower PFN of the buddy pair.
 * @param order The order of the block.
 * @return size_t The index in the pending merges array.
 */

size_t page_allocator_buddy::pending_merge_index(u64 lower_pfn, int order) { return (lower_pfn + order) % MAX_PENDING_MERGES; }

void page_allocator_buddy::set_pending_merge(u64 pfn, int order)
{
	size_t index = pending_merge_index(pfn, order);

	// Accesses the appropriate u64 in the pending_merges_ array and set the bit at the calculated index
	// through the creation of a bitmask and a bitwise OR operation.
	// I chose this specific way of setting the bit to ensure the highest possible performance
	// purely through bitwise operations.
	pending_merges_[order][index / 64] |= (1ULL << (index % 64));
}

/**
 * @brief Clears the pending merge for a given PFN and order.
 *
 * @param pfn The PFN of the block.
 * @param order The order of the block.
 * @return true if a pending merge was cleared, false otherwise.
 */

bool page_allocator_buddy::is_pending_merge(u64 pfn, int order)
{
	size_t index = pending_merge_index(pfn, order);

	// Accesses the appropriate u64 in the pending_merges_ array and checks if the bit at the calculated index is set
	// through the creation of a bitmask and a bitwise AND operation.
	// I chose this specific way of checking the bit to ensure the highest possible performance
	// purely through bitwise operations.
	return get_bit(order, index) == 1;
}

void page_allocator_buddy::clear_pending_merge(u64 pfn, int order)
{
	size_t index = pending_merge_index(pfn, order);

	// Mark the element at index as no longer pending in the order-th merge set.
	pending_merges_[order][index / 64] &= ~(1ULL << (index % 64));
}

/**
 * Accesses the appropriate u64 in the pending_merges_ array and checks if the bit at the calculated index is set
 * Otherwise, if it cannot find the bit, it returns 0.
 * @param order The order of the block.
 * @param idx The bit index (from get_pending_index).
 * @return 1 if the bit is set, 0 otherwise.
 */
int page_allocator_buddy::get_bit(int order, size_t idx) { return (pending_merges_[order][idx / 64] & (1ULL << (idx % 64))) ? 1 : 0; }

/**
 * @brief Cleans up pending merges by attempting to merge any blocks that are still eligible.
 * At first, it will check try to retrieve the bit from the pending merges array to see if there is a pending merge.
 * This function iterates through all orders and checks for pending merges. If both buddies are free and aligned,
 * it merges them and clears the pending merge mark. If not, it simply clears the pending mark.
 */

void page_allocator_buddy::cleanup_pending_merges()
{
	for (int order = 0; order <= LastOrder; ++order) {
		for (size_t idx = 0; idx < MAX_PENDING_MERGES; ++idx) {
			if (get_bit(order, idx)) {
				// Because of possible modulo collisions, we heuristically approximate the lower PFN.
				// by assigning it to the index. 
				u64 possible_lower_pfn = idx;
				// Verify the hash matches the expected index to avoid collisions.
				size_t expected_idx = get_pending_index(possible_lower_pfn, order);

				if (expected_idx == idx) {
					u64 buddy_pfn = calculate_other_buddy_pfn(order, possible_lower_pfn);
					page &buddy1 = page::get_from_pfn(possible_lower_pfn);
					page &buddy2 = page::get_from_pfn(buddy_pfn);
					if (block_aligned(order, buddy1.pfn()) && block_aligned(order, buddy2.pfn()) && is_buddy_free(order, buddy1.pfn())
						&& is_buddy_free(order, buddy2.pfn())) {
						merge_buddies(order, buddy1);
						clear_pending_merge(order, possible_lower_pfn);
					} else {
						// Clear invalid pending marks
						// Might happen because of some changes in the meantime or hash collisions
						dprintf("Clearing invalid pending merge for PFN %lu at order %d. Might have had collision\n", possible_lower_pfn, order);
						clear_pending_merge(order, possible_lower_pfn);
					}
				} else {
					// It could not match the hash, so we clear the pending merge mark and move on.
					clear_pending_merge(order, possible_lower_pfn);
				}
			}
		}
	}
}
