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

#include <internal/cute_transport_internal.h>
using namespace cute;

CUTE_TEST_CASE(test_sequence_buffer_basic, "Create sequence buffer, insert a few entries, find them, remove them.");
int test_sequence_buffer_basic()
{
	sequence_buffer_t sequence_buffer;
	sequence_buffer_t* buffer = &sequence_buffer;
	CUTE_TEST_CHECK(sequence_buffer_init(buffer, 256, sizeof(int), NULL));

	int entries[3];
	int count = sizeof(entries) / sizeof(entries[0]);
	for (int i = 0; i < count; ++i)
	{
		int* entry = (int*)sequence_buffer_insert(buffer, i);
		CUTE_TEST_CHECK_POINTER(entry);
		*entry = entries[i] = i;
	}

	for (int i = 0; i < count; ++i)
	{
		int* entry = (int*)sequence_buffer_find(buffer, i);
		CUTE_TEST_CHECK_POINTER(entry);
		CUTE_TEST_ASSERT(*entry == entries[i]);
	}

	for (int i = 0; i < count; ++i)
	{
		sequence_buffer_remove(buffer, i);
		int* entry = (int*)sequence_buffer_find(buffer, i);
		CUTE_TEST_ASSERT(entry == NULL);
	}

	sequence_buffer_cleanup(buffer);

	return 0;
}
