/*
	Cute Framework
	Copyright (C) 2020 Randy Gaul https://randygaul.net

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

#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <cute.h>
using namespace cute;

cute::error_t serialize_v2(kv_t* kv, const char* key, v2* v);
cute::error_t serialize_rotation(kv_t* kv, const char* key, rotation_t* rotation);
cute::error_t serialize_transform(kv_t* kv, const char* key, transform_t* transform);
cute::error_t kv_val(kv_t* kv, const char** string);

#endif // SERIALIZE_H
