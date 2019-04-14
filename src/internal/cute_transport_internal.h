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

#ifndef CUTE_TRANSPORT_INTERNAL_H
#define CUTE_TRANSPORT_INTERNAL_H

#include <cute_defines.h>

namespace cute
{

struct sequence_buffer_t
{
	uint16_t sequence;
	int capacity;
	int stride;
	uint32_t* entry_sequence;
	uint8_t* entry_data;
	void* mem_ctx;
};

extern CUTE_API int CUTE_CALL sequence_buffer_init(sequence_buffer_t* buffer, int capacity, int stride, void* mem_ctx);
extern CUTE_API void CUTE_CALL sequence_buffer_cleanup(sequence_buffer_t* buffer);
extern CUTE_API void CUTE_CALL sequence_buffer_reset(sequence_buffer_t* buffer);

typedef void (sequence_buffer_cleanup_entry_fn)(void* data, void* mem_ctx);

extern CUTE_API void CUTE_CALL sequence_buffer_advance(sequence_buffer_t* buffer, uint16_t sequence);
extern CUTE_API void* CUTE_CALL sequence_buffer_insert(sequence_buffer_t* buffer, uint16_t sequence, sequence_buffer_cleanup_entry_fn* cleanup_fn = NULL);
extern CUTE_API void CUTE_CALL sequence_buffer_remove(sequence_buffer_t* buffer, uint16_t sequence, sequence_buffer_cleanup_entry_fn* cleanup_fn = NULL);
extern CUTE_API int CUTE_CALL sequence_buffer_is_empty(sequence_buffer_t* sequence_buffer, uint16_t sequence);
extern CUTE_API void* CUTE_CALL sequence_buffer_find(sequence_buffer_t* sequence_buffer, uint16_t sequence);
extern CUTE_API void* CUTE_CALL sequence_buffer_at_index(sequence_buffer_t* sequence_buffer, int index);
extern CUTE_API void CUTE_CALL sequence_buffer_generate_ack_bits(sequence_buffer_t* sequence_buffer, uint16_t* ack, uint32_t* ack_bits);

// -------------------------------------------------------------------------------------------------

}

#endif // CUTE_TRANSPORT_INTERNAL_H
