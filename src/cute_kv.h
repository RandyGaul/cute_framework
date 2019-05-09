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

// TODO : Audit error reporting strategy. Probably majority of functions can fail.

namespace cute
{

struct kv_t;

#define CUTE_KV_MODE_WRITE 1
#define CUTE_KV_MODE_READ  0

CUTE_API kv_t* CUTE_CALL kv_make(void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL kv_destroy(kv_t* kv);
CUTE_API error_t CUTE_CALL kv_reset(kv_t* kv, const void* data, int size, int mode);

CUTE_API void CUTE_CALL kv_peek_object(kv_t* kv, const char** str, int* len);
CUTE_API void CUTE_CALL kv_object_begin(kv_t* kv, const char* key, const char* type_id);
CUTE_API error_t CUTE_CALL kv_object_end(kv_t* kv);

CUTE_API void CUTE_CALL kv_field(kv_t* kv, const char* key, uint8_t* val);
CUTE_API void CUTE_CALL kv_field(kv_t* kv, const char* key, uint16_t* val);
CUTE_API void CUTE_CALL kv_field(kv_t* kv, const char* key, uint32_t* val);
CUTE_API void CUTE_CALL kv_field(kv_t* kv, const char* key, uint64_t* val);

CUTE_API void CUTE_CALL kv_field(kv_t* kv, const char* key, int8_t* val);
CUTE_API void CUTE_CALL kv_field(kv_t* kv, const char* key, int16_t* val);
CUTE_API void CUTE_CALL kv_field(kv_t* kv, const char* key, int32_t* val);
CUTE_API void CUTE_CALL kv_field(kv_t* kv, const char* key, int64_t* val);

CUTE_API void CUTE_CALL kv_field(kv_t* kv, const char* key, float* val);
CUTE_API void CUTE_CALL kv_field(kv_t* kv, const char* key, double* val);

CUTE_API void CUTE_CALL kv_field_str(kv_t* kv, const char* key, char** str, int* size);
CUTE_API void CUTE_CALL kv_field_blob(kv_t* kv, const char* key, void* data, int* size);
CUTE_API void CUTE_CALL kv_field_array_begin(kv_t* kv, const char* key, int* count, const char* type_id = NULL);
CUTE_API void CUTE_CALL kv_field_array_end(kv_t* kv);

CUTE_API void CUTE_CALL kv_print(kv_t* kv);

}

#endif // CUTE_KV_H
