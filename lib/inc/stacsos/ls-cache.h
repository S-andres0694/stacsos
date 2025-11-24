#pragma once

/* Simple LS cache type
 *
 * This provides a cache that maps the path to the expected
 * final product struct for that path.
 *
 * It aims to increase the performance of repeated 'ls' calls
 * on the same directory path by caching the results.
 */

#include <stacsos/list.h>
#include <stacsos/map.h>
#include <stacsos/string.h>
#include <stacsos/syscalls.h>

namespace stacsos {
class ls_cache {
public:
	using key_t = u64; // Using the hash of the string as the key. Originally, tried using a char array but ran into issues with
	// the lookups not working properly.
	using value_t = final_product;
	using map_t = stacsos::map<key_t, value_t>;

	ls_cache()
		: map_()
		, keys_()
		, max_size_(8) { };

	/**
	 * Inserts or updates a cache entry.
	 * If the cache exceeds its maximum size, the oldest entry is evicted.
	 * Similar to the idea of an LRU cache.
	 * @param name The directory path.
	 * @param entry The final_product to cache.
	 * @return true if the entry was added/updated successfully.
	 */

	bool put(string &name, value_t entry)
	{
		key_t k = key_t(name.get_hash());
		if (count >= max_size_) {
			// Evict the oldest entry
			key_t oldest_key = keys_.dequeue();
			map_.erase(oldest_key);
			count--;
		}

		map_.add(k, entry);
		keys_.enqueue(k);
		count++;
		return true;
	}

	/**
	 * Looks up a cache entry by name.
	 * @param name The directory path.
	 * @param out Reference to store the found cache entry.
	 * @return true if the entry was found, false otherwise.
	 */

	bool lookup(string &name, value_t &out)
	{
		key_t k = key_t(name.get_hash());
		return map_.try_get_value(k, out);
	}

private:
	map_t map_; // The underlying map storing cache entries
	list<key_t> keys_; // List of keys for managing the oldest entries
	size_t max_size_ = 8; // Maximum number of cache entries. Number chose arbitrarily with hope of balancing memory use and cache hits.
	u8 count = 0; // Current number of entries. I use this instead of counting
	// through the map for performance reasons.
};
} // namespace stacsos