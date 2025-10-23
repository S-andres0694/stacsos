/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024, 2025
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <csignal>
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
	// Make sure that the amount of pages being inserted is non-zero.
	assert(page_count > 0);

	page *current_page = &range_start;

	// The idea is to basically dynamically convert each page range into its
	// corresponding block with the intention of inserting them into the free
	// list.
	for (int curr_order = LastOrder; curr_order >= 0; curr_order--) {
		u8 block_size = block_size_per_order(curr_order);
		while (block_aligned(curr_order, current_page->pfn()) && page_count >= block_size) {
			// By checking if the block starting from the current page is aligned
			// and that there are enough pages to insert based on the block size
			// I can insert the block of pages starting from the current page.
			insert_free_block(curr_order, *current_page);

			// For optimizations, I make sure to attempt a coalescence in the order
			// which makes sure that fragmentation is as small as possible and that
			// we have the biggest amount of large continuous blocks.
			merge_buddies(curr_order, *current_page);

			// Then, I get the next starting page of the next block by adding the
			// current block size (the amount of pages we inserted) to the PFN of
			// the current starting page.
			current_page = &page::get_from_pfn(current_page->pfn() + block_size);
			page_count -= block_size;
		}
	}

	// Final update to the metadata, as we added all of the
	// free pages back to the allocator.
	total_free_ += page_count;
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
void page_allocator_buddy::split_block(int order, page &block_start) { panic("TODO"); }

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
	assert(0 < order && order < LastOrder);

	// We know that the page that was given has
	// to be free if is going to be merged
	// as said by the spec (Page 4).

	// Get the buddy candidate.
	page &other_buddy = page::get_from_pfn(calculate_other_buddy_pfn(order, buddy.pfn()));

	// I make a series of checks to make sure that the candidate and
	// passed block are actually buddies.

	// They must both be aligned to the passed order.
	assert(block_aligned(order, buddy.pfn()) && block_aligned(order, other_buddy.pfn()));

	// Because they are going to be merged, they can no longer be free.
	remove_free_block(order, buddy);
	remove_free_block(order, other_buddy);

	// Create the newly merged block by choosing the starting page
	// based on the smallest PFN.
	u64 merged_pfn = (buddy.pfn() < other_buddy.pfn()) ? buddy.pfn() : other_buddy.pfn(); 
	page &merged_block = page::get_from_pfn(merged_pfn);

	//Insert the merged block into the next higher order 
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
page *page_allocator_buddy::allocate_pages(int order, page_allocation_flags flags) { panic("TODO"); }

/**
 * @brief Frees previously allocated pages, using the buddy algorithm.
 *
 * ** You are required to implement this function **
 * @param block_start The starting page of the block to be freed.
 * @param order The order of the block being freed.
 */
void page_allocator_buddy::free_pages(page &block_start, int order) { panic("TODO"); }

/**
 * @brief Calculates the size of the block based on the current order.
 * I employed a bitwise shift operation because it is more performant than
 * a multiplication.
 *
 * @param order - the order for which the calculation is going to be done.
 * @return u8 the amount of pages that can go inside of the block.
 */

u8 page_allocator_buddy::block_size_per_order(int order) { return 1 << order; }

/**
 * @brief Calculates the PFN of the other buddy in the same order.
 *
 * @param order The order of the block.
 * @param buddy_pfn The PFN of the given buddy block.
 * @return The PFN of the other buddy block.
 */
static u64 calculate_other_buddy_pfn(int order, u64 buddy_pfn) { return buddy_pfn ^ (1 << order); }