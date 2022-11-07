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

#ifndef CUTE_KV_UTILS_H
#define CUTE_KV_UTILS_H

#include "cute_defines.h"

#ifdef CUTE_CPP

#include "cute_kv.h"
#include "cute_ecs.h"
#include "cute_string.h"

#include <string>
#include <vector>

CUTE_INLINE cf_result_t cf_kv_val_string_cf(cf_kv_t* kv, cute::string_t* string)
{
	const char* ptr = string->c_str();
	size_t len = string->len();
	cf_result_t err = cf_kv_val_string(kv, &ptr, &len);
	if (cf_is_error(err)) return err;
	*string = cute::string_t(ptr, ptr + len);
	return cf_result_success();
}

CUTE_INLINE cf_result_t cf_kv_val_string_std(cf_kv_t* kv, std::string* val)
{
	const char* ptr = val->data();
	size_t len = val->length();
	cf_result_t err = cf_kv_val_string(kv, &ptr, &len);
	if (cf_is_error(err)) return err;
	val->assign(ptr, len);
	return cf_result_success();
}

template <typename T>
CUTE_INLINE cf_result_t cf_kv_val_vec(cf_kv_t* kv, std::vector<T>* val, const char* key = NULL)
{
	int count = (int)val->size();
	cf_kv_array_begin(kv, &count, key);
	val->resize(count);
	for (int i = 0; i < count; ++i) {
		cf_kv_val_int32(kv, &(*val)[i]);
	}
	cf_kv_array_end(kv);
	return cf_kv_error_state(kv);
}

namespace cute
{

CUTE_INLINE result_t kv_val_cf_string(kv_t* kv, string_t* string) { return cf_kv_val_string_cf(kv, string); }
CUTE_INLINE result_t kv_val(kv_t* kv, std::string* val) { return cf_kv_val_string_std(kv, val); }

}
#endif // CUTE_CPP

#endif // CUTE_KV_UTILS_H
