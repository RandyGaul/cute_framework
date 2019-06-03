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

#ifndef CUTE_KV_H
#define CUTE_KV_H

#include <cute_defines.h>
#include <cute_error.h>

namespace cute
{

struct kv_t;

#define CUTE_KV_MODE_WRITE 1
#define CUTE_KV_MODE_READ  0

CUTE_API kv_t* CUTE_CALL kv_make(void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL kv_destroy(kv_t* kv);
CUTE_API error_t CUTE_CALL kv_reset_io(kv_t* kv, const void* data, int size, int mode);
CUTE_API void CUTE_CALL kv_reset_read(kv_t* kv);
CUTE_API int CUTE_CALL kv_size_written(kv_t* kv);
CUTE_API error_t CUTE_CALL kv_error_state(kv_t* kv);

enum kv_type_t
{
	KV_TYPE_NULL   = 0,
	KV_TYPE_INT64  = 1,
	KV_TYPE_DOUBLE = 2,
	KV_TYPE_STRING = 3,
	KV_TYPE_ARRAY  = 4,
	KV_TYPE_OBJECT = 5,
};

CUTE_API error_t CUTE_CALL kv_key(kv_t* kv, const char* key, kv_type_t* type = NULL);

CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, uint8_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, uint16_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, uint32_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, uint64_t* val);

CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, int8_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, int16_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, int32_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, int64_t* val);

CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, float* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, double* val);

CUTE_API error_t CUTE_CALL kv_val_string(kv_t* kv, char** str, int* size);
CUTE_API error_t CUTE_CALL kv_val_blob(kv_t* kv, void* data, int* size, int capacity);

CUTE_API error_t CUTE_CALL kv_object_begin(kv_t* kv);
CUTE_API error_t CUTE_CALL kv_object_end(kv_t* kv);

CUTE_API error_t CUTE_CALL kv_array_begin(kv_t* kv, int* count);
CUTE_API error_t CUTE_CALL kv_array_end(kv_t* kv);

CUTE_API void CUTE_CALL kv_print(kv_t* kv);

}

#endif // CUTE_KV_H

#include <cute_kv_utils.inl>
