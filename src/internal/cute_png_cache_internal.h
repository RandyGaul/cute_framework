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

#ifndef CUTE_PNG_CACHE_INTERNAL_H
#define CUTE_PNG_CACHE_INTERNAL_H

#include <cute_defines.h>
#include <cute_array.h>
#include <cute_dictionary.h>
#include <cute_strpool.h>

namespace cute
{

struct strpool_t;

struct png_cache_t
{
	dictionary<uint64_t, png_t> pngs;
	dictionary<uint64_t, void*> id_to_pixels;
	dictionary<strpool_id, animation_t*> animations;
	dictionary<strpool_id, animation_table_t*> animation_tables;
	uint64_t id_gen = 0;
	strpool_t* strpool = NULL;
	void* mem_ctx = NULL;
};

}

#endif // CUTE_PNG_CACHE_INTERNAL_H
