/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <cute_hashtable.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>
#include <cute_string.h>
#include <cute_array.h>

using namespace cute;

// Original implementation by Mattias Gustavsson
// https://github.com/mattiasgustavsson/libs/blob/main/hashtable.h

// Prime table sizes are used to help security of the table at the expense of the % operator,
// as opposed to the speed << operator, for lookups.
// By using prime table sizes we ensure all bits of each hash are utilized. This helps mitigate
// DoS attacks on the table lookup function.
static int s_primes[] = {
	31, 67, 127, 257, 509, 1021, 2053, 4099, 8191, 16381, 32771, 65537, 131071,
	262147, 524287, 1048573, 2097143, 4194301, 8388617, 16777213, 33554467,
	67108859, 134217757, 268435459, 536870909, 1073741827, 2147483647
};

static CUTE_INLINE int s_next_prime(int a)
{
	int i = 0;
	while (s_primes[i] <= a) ++i;
	return s_primes[i];
}

static CUTE_INLINE void* s_get_item(const cf_hhdr_t* table, int index)
{
	uint8_t* items = (uint8_t*)table->items_data;
	return items + index * table->item_size;
}

void* cf_hashtable_make_impl(int key_size, int item_size, int capacity)
{
	CUTE_ASSERT(capacity);

	cf_hhdr_t* table = (cf_hhdr_t*)CUTE_CALLOC(sizeof(cf_hhdr_t) + (capacity + 1) * item_size);
	table->cookie = HCOOKIE;
	table->key_size = key_size;
	table->item_size = item_size;

	// Space is made for a zero'd out "hidden item" to represent failed lookups.
	// This is critical to support return-by-value polymorphism in the C macro API for `hget` and `hfind`.
	// We also "pass" in values to `hadd` through this space.
	table->hidden_item = (void*)((uintptr_t)(table + 1));
	table->items_data = (void*)((uintptr_t)(table + 1) + item_size);
	table->slot_capacity = s_next_prime(capacity);
	table->slots = (cf_hslot_t*)CUTE_CALLOC(table->slot_capacity * sizeof(cf_hslot_t));
	for (int i = 0; i < table->slot_capacity; ++i) {
		table->slots[i].item_index = -1;
	}
	table->item_capacity = capacity;
	table->items_key = CUTE_ALLOC(capacity * item_size);
	table->items_slot_index = (int*)CUTE_ALLOC(capacity * sizeof(*table->items_slot_index));
	table->temp_key = CUTE_ALLOC(key_size);
	table->temp_item = CUTE_ALLOC(item_size);

	return s_get_item(table, 0);
}

void cf_hashtable_free_impl(cf_hhdr_t* table)
{
	if (!table) return;
	CUTE_FREE(table->slots);
	CUTE_FREE(table->items_key);
	CUTE_FREE(table->items_slot_index);
	CUTE_FREE(table->temp_key);
	CUTE_FREE(table->temp_item);
	CUTE_FREE(table);
}

static CUTE_INLINE int s_keys_equal(const cf_hhdr_t* table, const void* a, const void* b)
{
	return !CUTE_MEMCMP(a, b, table->key_size);
}

static CUTE_INLINE void* s_get_key(const cf_hhdr_t* table, int index)
{
	uint8_t* keys = (uint8_t*)table->items_key;
	return keys + index * table->key_size;
}

static int s_find_slot(const cf_hhdr_t *table, uint32_t hash, const void* key)
{
	uint32_t slot_capacity = (uint32_t)table->slot_capacity;
	int base_slot = (int)(hash % slot_capacity);
	int base_count = table->slots[base_slot].base_count;
	int slot = base_slot;

	while (base_count > 0) {
		if (table->slots[slot].item_index >= 0) {
			uint32_t slot_hash = table->slots[slot].key_hash;
			int slot_base = (int)(slot_hash % slot_capacity);
			if (slot_base == base_slot) {
				CUTE_ASSERT(base_count > 0);
				--base_count;
				const void* slot_key = s_get_key(table, table->slots[slot].item_index);
				if (slot_hash == hash && s_keys_equal(table, slot_key, key)) {
					return slot;
				}
			}
		}
		slot = (int)((slot + 1) % table->slot_capacity);
	}

	return -1;
}

static void s_expand_slots(cf_hhdr_t* table)
{
	int old_capacity = table->slot_capacity;
	cf_hslot_t* old_slots = table->slots;

	table->slot_capacity = s_next_prime(table->slot_capacity);
	uint32_t slot_capacity = (uint32_t)table->slot_capacity;

	table->slots = (cf_hslot_t*)CUTE_CALLOC(table->slot_capacity * sizeof(cf_hslot_t));
	CUTE_ASSERT(table->slots);
	for (int i = 0; i < table->slot_capacity; ++i) {
		table->slots[i].item_index = -1;
	}

	for (int i = 0; i < old_capacity; ++i) {
		if (old_slots[i].item_index >= 0) {
			uint32_t hash = old_slots[i].key_hash;
			int base_slot = (int)(hash % slot_capacity);
			int slot = base_slot;
			while (table->slots[slot].item_index >= 0) {
				slot = (slot + 1) % slot_capacity;
			}
			table->slots[slot].key_hash = hash;
			int item_index = old_slots[i].item_index;
			table->slots[slot].item_index = item_index;
			table->items_slot_index[item_index] = slot; 
			++table->slots[base_slot].base_count;
		}
	}

	CUTE_FREE(old_slots);
}

static cf_hhdr_t* s_expand_items(cf_hhdr_t* table)
{
	int capacity = table->item_capacity * 2;
	table = (cf_hhdr_t*)CUTE_REALLOC(table, sizeof(cf_hhdr_t) + (capacity + 1) * table->item_size);
	table->item_capacity = capacity;
	table->hidden_item = (void*)((uintptr_t)(table + 1));
	table->items_data = (void*)((uintptr_t)(table + 1) + table->item_size);
	table->items_key = CUTE_REALLOC(table->items_key, capacity * table->key_size);
	table->items_slot_index = (int*)CUTE_REALLOC(table->items_slot_index, capacity * sizeof(*table->items_slot_index));
	return table;
}

void* cf_hashtable_insert_impl2(cf_hhdr_t* table, const void* key, const void* item)
{
	uint32_t hash = (uint32_t)fnv1a(key, table->key_size);
	CUTE_ASSERT(s_find_slot(table, hash, key) < 0);

	if (table->count >= (table->slot_capacity - table->slot_capacity / 3)) {
		s_expand_slots(table);
	}

	uint32_t slot_capacity = (uint32_t)table->slot_capacity;
	int base_slot = (int)(hash % slot_capacity);
	int base_count = table->slots[base_slot].base_count;
	int slot = base_slot;
	int first_free = slot;
	while (base_count) {
		if (table->slots[slot].item_index < 0 && table->slots[first_free].item_index >= 0) first_free = slot;
		uint32_t slot_hash = table->slots[slot].key_hash;
		int slot_base = (int)(slot_hash % slot_capacity);
		if (slot_base == base_slot) {
			--base_count;
		}
		slot = (slot + 1) % table->slot_capacity;
	}

	slot = first_free;
	while (table->slots[slot].item_index >= 0) {
		slot = (slot + 1) % slot_capacity;
	}

	if (table->count >= table->item_capacity) {
		table = s_expand_items(table);
	}

	CUTE_ASSERT(table->count < table->item_capacity);
	CUTE_ASSERT(table->slots[slot].item_index < 0 && (hash % slot_capacity) == (uint32_t)base_slot);
	table->slots[slot].key_hash = hash;
	table->slots[slot].item_index = table->count;
	++table->slots[base_slot].base_count;

	void* item_dst = s_get_item(table, table->count);
	void* key_dst = s_get_key(table, table->count);
	if (item) CUTE_MEMCPY(item_dst, item, table->item_size);
	CUTE_MEMCPY(key_dst, key, table->key_size);
	table->items_slot_index[table->count] = slot;
	table->return_index = table->count++;

	return s_get_item(table, 0);
}

void* cf_hashtable_insert_impl3(cf_hhdr_t* table, const void* key)
{
	return cf_hashtable_insert_impl2(table, key, table->hidden_item);
}

void* cf_hashtable_insert_impl(cf_hhdr_t* table, uint64_t key)
{
	return cf_hashtable_insert_impl2(table, &key, table->hidden_item);
}

void cf_hashtable_remove_impl2(cf_hhdr_t* table, const void* key)
{
	uint32_t hash = (uint32_t)fnv1a(key, table->key_size);
	int slot = s_find_slot(table, hash, key);
	CUTE_ASSERT(slot >= 0);

	int base_slot = (int)(hash % (uint32_t)table->slot_capacity);
	int index = table->slots[slot].item_index;
	int last_index = table->count - 1;
	--table->slots[base_slot].base_count;
	table->slots[slot].item_index = -1;

	if (index != last_index) {
		void* dst_key = s_get_key(table, index);
		void* src_key = s_get_key(table, last_index);
		CUTE_MEMCPY(dst_key, src_key, (size_t)table->key_size);
		void* dst_item = s_get_item(table, index);
		void* src_item = s_get_item(table, last_index);
		CUTE_MEMCPY(dst_item, src_item, (size_t)table->item_size);
		table->items_slot_index[index] = table->items_slot_index[last_index];
		table->slots[table->items_slot_index[last_index]].item_index = index;
	}
	--table->count;
}

void cf_hashtable_remove_impl(cf_hhdr_t* table, uint64_t key)
{
	cf_hashtable_remove_impl2(table, &key);
}

void cf_hashtable_clear_impl(cf_hhdr_t* table)
{
	table->count = 0;
	for (int i = 0; i < table->slot_capacity; ++i) {
		table->slots[i].base_count = 0;
		table->slots[i].item_index = -1;
	}
}

int cf_hashtable_find_impl2(const cf_hhdr_t* table, const void* key)
{
	int slot = s_find_slot(table, (uint32_t)fnv1a(key, table->key_size), key);
	if (slot < 0) {
		// We will be "returning" a zero'd out item through `hget` with this
		// hidden item.
		CUTE_MEMSET(table->hidden_item, 0, table->item_size);
		return -1;
	}
	int index = table->slots[slot].item_index;
	return index;
}

int cf_hashtable_find_impl(const cf_hhdr_t* table, uint64_t key)
{
	return cf_hashtable_find_impl2(table, &key);
}

bool cf_hashtable_has_impl(cf_hhdr_t* table, uint64_t key)
{
	return !!cf_hashtable_find_impl(table, key);
}

int cf_hashtable_count_impl(const cf_hhdr_t* table)
{
	return table->count;
}

void* cf_hashtable_items_impl(const cf_hhdr_t* table)
{
	return table->items_data;
}

void* cf_hashtable_keys_impl(const cf_hhdr_t* table)
{
	return table->items_key;
}

void cf_hashtable_swap_impl(cf_hhdr_t* table, int index_a, int index_b)
{
	if (index_a < 0 || index_a >= table->count || index_b < 0 || index_b >= table->count) return;

	int slot_a = table->items_slot_index[index_a];
	int slot_b = table->items_slot_index[index_b];

	table->items_slot_index[index_a] = slot_b;
	table->items_slot_index[index_b] = slot_a;

	void* key_a = s_get_key(table, index_a);
	void* key_b = s_get_key(table, index_b);
	CUTE_MEMCPY(table->temp_key, key_a, table->key_size);
	CUTE_MEMCPY(key_a, key_b, table->key_size);
	CUTE_MEMCPY(key_b, table->temp_key, table->key_size);

	void* item_a = s_get_item(table, index_a);
	void* item_b = s_get_item(table, index_b);
	CUTE_MEMCPY(table->temp_item, item_a, table->item_size);
	CUTE_MEMCPY(item_a, item_b, table->item_size);
	CUTE_MEMCPY(item_b, table->temp_item, table->item_size);

	table->slots[slot_a].item_index = index_b;
	table->slots[slot_b].item_index = index_a;
}
