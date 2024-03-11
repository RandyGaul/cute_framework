/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "cute_array.h"
#include "cute_math.h"

#include <internal/cute_alloc_internal.h>

using namespace Cute;

void* cf_agrow(const void* a, int new_size, size_t element_size)
{
	CF_ACANARY(a);
	CF_ASSERT(acap(a) <= (SIZE_MAX - 1)/2);
	int new_capacity = max(2 * acap(a), max(new_size, 16));
	CF_ASSERT(new_size <= new_capacity);
	CF_ASSERT(new_capacity <= (SIZE_MAX - sizeof(CF_Ahdr)) / element_size);
	size_t total_size = sizeof(CF_Ahdr) + new_capacity * element_size;
	CF_Ahdr* hdr;
	if (a) {
		if (!CF_AHDR(a)->is_static) {
			hdr = (CF_Ahdr*)CF_REALLOC(CF_AHDR(a), total_size);
		} else {
			hdr = (CF_Ahdr*)CF_ALLOC(total_size);
			CF_MEMCPY(hdr + 1, a, alen(a) * element_size);
			hdr->size = asize(a);
			hdr->cookie = CF_ACOOKIE;
		}
	} else {
		hdr = (CF_Ahdr*)CF_ALLOC(total_size);
		hdr->size = 0;
		hdr->cookie = CF_ACOOKIE;
	}
	hdr->capacity = new_capacity;
	hdr->is_static = false;
	hdr->data = (char*)(hdr + 1); // For debugging convenience.
	return (void*)(hdr + 1);
}

void* cf_astatic(const void* a, int buffer_size, size_t element_size)
{
	CF_Ahdr* hdr = (CF_Ahdr*)a;
	hdr->size = 0;
	hdr->cookie = CF_ACOOKIE;
	if (sizeof(CF_Ahdr) <= element_size) {
		hdr->capacity = buffer_size / (int)element_size - 1;
	} else {
		int elements_taken = sizeof(CF_Ahdr) / (int)element_size + (sizeof(CF_Ahdr) % (int)element_size > 0);
		hdr->capacity = buffer_size / (int)element_size - elements_taken;
	}
	hdr->data = (char*)(hdr + 1); // For debugging convenience.
	hdr->is_static = true;
	return (void*)(hdr + 1);
}

void* cf_aset(const void* a, const void* b, size_t element_size)
{
	CF_ACANARY(a);
	CF_ACANARY(b);
	if (acap(a) < asize(b)) {
		int len = asize(b);
		a = cf_agrow(a, asize(b), element_size);
	}
	CF_MEMCPY((void*)a, b, asize(b) * element_size);
	alen(a) = asize(b);
	return (void*)a;
}

void* cf_arev(const void* a_ptr, size_t element_size)
{
	CF_ACANARY(a_ptr);
	void* a = (void*)a_ptr;
	int ia = 0;
	int ib = acount(a) - 1;
	void* t = CF_ALLOC(element_size); // Maybe use alloca? malloc won't cause stack overflow.
	void* b = (void*)((uintptr_t)a + (element_size * ib));

	while (ia < ib) {
		CF_MEMCPY(t, a, element_size);
		CF_MEMCPY(a, b, element_size);
		CF_MEMCPY(b, t, element_size);
		a = (void*)((uintptr_t)a + (element_size * ia++));
		b = (void*)((uintptr_t)b + (element_size * ib++));
	}

	CF_FREE(t);

	return (void*)a_ptr;
}
