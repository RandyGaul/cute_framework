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

#ifndef LAMP_H
#define LAMP_H

#include <cute.h>
using namespace cute;

struct Lamp
{
	int oil_count = 10;
	int oil_capacity = 10;
	bool is_held = false;
};

CUTE_INLINE cute::error_t Lamp_serialize(app_t* app, kv_t* kv, entity_t entity, void* component, void* udata)
{
	Lamp* lamp = (Lamp*)component;
	if (kv_get_state(kv) == KV_STATE_READ) {
		CUTE_PLACEMENT_NEW(lamp) Lamp;
	}
	return kv_error_state(kv);
}

#endif // LAMP_H
