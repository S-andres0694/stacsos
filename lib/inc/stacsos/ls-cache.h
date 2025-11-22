#pragma once

/* Simple LS cache type 
 *
 * This provides a cache that maps the path to the expected 
 * final product struct for that path.
 * 
 * It aims to increase the performance of repeated 'ls' calls
 * on the same directory path by caching the results.
 */

#include <stacsos/map.h>
#include <stacsos/syscalls.h>
#include <stacsos/list.h>

namespace stacsos {

class ls_cache {
public:
	using key_t = const char *;
	using value_t = cache_entry;
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

	bool put(const char *name, value_t entry)
	{
		key_t k(name);
		if (map_.size() >= max_size_) {
            // Evict the oldest entry
            const char *oldest_key = keys_.dequeue();
			map_.erase(oldest_key);
        }

        map_.add(k, entry);
        keys_.enqueue(k);
        return true;
	}

	/**
     * Looks up a cache entry by name.
     * @param name The directory path.
     * @param out Reference to store the found cache entry.
     * @return true if the entry was found, false otherwise.
     */

	bool lookup(const char *name, value_t &out) 
	{
		key_t k(name);
		return map_.try_get_value(k, out);
	}

    /**
     * Gets the dirty bit for a given cache entry.
     * If the entry does not exist, return false.
     * @param name The directory path.
     * @return The dirty bit value.
     */

	bool get_dirty_bit(const char *name) 
	{
		cache_entry entry;
		if (map_.try_get_value(name, entry)) {
			return entry.dirty_bit;
		}
		return false; 
	}

    /**
     * Sets the dirty bit for a given cache entry.
     * If the entry does not exist, return false.
     * @param name The directory path.
     * @param value The value to set the dirty bit to.
     * @return true if the dirty bit was set successfully, false otherwise.
     */

	bool set_dirty_bit(const char *name, bool value = true)
	{
		cache_entry entry;
		if (!map_.try_get_value(name, entry)) {
			return false; // no such entry
		}

		entry.dirty_bit = value;

		// Replace the existing entry in the map
		map_.add(name, entry);
		return true;
	}

private:
	map_t map_; // The underlying map storing cache entries
    list<const char *> keys_; // List of keys for managing the oldest entries
    size_t max_size_ = 8; // Maximum number of cache entries. Number chose arbitrarily with hope of balancing memory use and cache hits.
};

} 