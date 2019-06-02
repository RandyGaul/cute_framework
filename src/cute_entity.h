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

#ifndef CUTE_ENTITY_H
#define CUTE_ENTITY_H

#include <cute_handle_table.h>

#define CUTE_ENTITY_MAX_COMPONENTS (16)

namespace cute
{

using entity_id_t = handle_t;
using entity_type_t = uint32_t;
using component_id_t = handle_t;
using component_type_t = uint32_t;

struct entity_t
{
	entity_type_t type;
	int component_count;
	component_id_t component_id[CUTE_ENTITY_MAX_COMPONENTS];
	component_type_t component_type[CUTE_ENTITY_MAX_COMPONENTS];
};

}

#endif // CUTE_ENTITY_H
