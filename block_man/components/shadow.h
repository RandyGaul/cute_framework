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

#ifndef SHADOW_H
#define SHADOW_H

#include <cute.h>
using namespace cute;

#include <serialize.h>

struct Shadow
{
	bool visible = true;
	bool tiny = false;
	bool small = false;
	bool big = false;
};

CUTE_INLINE cute::error_t Shadow_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	Shadow* shadow = (Shadow*)component;
	if (reading) {
		CUTE_PLACEMENT_NEW(shadow) Shadow;
	}
	kv_key(kv, "visible"); kv_val(kv, &shadow->visible);
	kv_key(kv, "tiny"); kv_val(kv, &shadow->tiny);
	kv_key(kv, "small"); kv_val(kv, &shadow->small);
	kv_key(kv, "big"); kv_val(kv, &shadow->big);
	return kv_error_state(kv);
}

#endif // SHADOW_H
