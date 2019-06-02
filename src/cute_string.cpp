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

#include <cute_string.h>
#include <cute_alloc.h>

#include <internal/cute_app_internal.h>

#define STRPOOL_IMPLEMENTATION
#define STRPOOL_MALLOC(ctx, size) CUTE_ALLOC(size, ctx)
#define STRPOOL_FREE(ctx, ptr) CUTE_FREE(ptr, ctx)
#include <mattiasgustavsson/strpool.h>

namespace cute
{

//--------------------------------------------------------------------------------------------------
// Global `string` pool instance.

static void* s_mem_ctx;
static bool s_pool_init;
static strpool_t s_pool_instance;

static strpool_t* s_pool(int nuke = 0)
{
	strpool_t* instance = &s_pool_instance;
	if (nuke) {
		strpool_term(instance);
		s_pool_init = 0;
		return NULL;
	} else {
		if (!s_pool_init) {
			strpool_init(instance, &strpool_default_config);
			s_pool_init = 1;
		}
		return instance;
	}
}

static void s_incref(uint64_t id)
{
	if (id) {
		strpool_t* pool = s_pool();
		strpool_incref(pool, id);
	}
}

static void s_decref(uint64_t id)
{
	if (id) {
		strpool_t* pool = s_pool();
		if (strpool_decref(pool, id) <= 0) {
			strpool_discard(pool, id);
		}
	}
}

//--------------------------------------------------------------------------------------------------

string_t::string_t()
	: id(0)
{
}

string_t::string_t(char* str)
{
	int len = strlen(str);
	if (len) {
		id = strpool_inject(s_pool(), str, len);
		s_incref(id);
	} else id = 0;
}

string_t::string_t(const char* str)
{
	int len = str ? strlen(str) : 0;
	if (len) {
		id = strpool_inject(s_pool(), str, len);
		s_incref(id);
	} else id = 0;
}

string_t::string_t(const char* begin, const char* end)
{
	int len = begin ? (end ? end - begin : strlen(begin)) : 0;
	if (len) {
		id = strpool_inject(s_pool(), begin, len);
		s_incref(id);
	} else id = 0;
}

string_t::string_t(const string_t& other)
	: id(other.id)
{
	s_incref(id);
}

string_t::~string_t()
{
	s_decref(id);
}

int string_t::len() const
{
	return strpool_length(s_pool(), id);
}

const char* string_t::c_str() const
{
	return strpool_cstr(s_pool(), id);
}

string_t& string_t::operator=(const string_t& rhs)
{
	s_incref(rhs.id);
	s_decref(id);
	id = rhs.id;
	return *this;
}

int string_t::operator==(const string_t& rhs) const
{
	return id == rhs.id;
}

int string_t::operator!=(const string_t& rhs) const
{
	return id != rhs.id;
}

void string_set_allocator_context(void* user_allocator_context)
{
	s_mem_ctx = user_allocator_context;
}

void* string_get_allocator_context()
{
	return s_mem_ctx;
}

void string_defrag()
{
	strpool_defrag(s_pool());
}

void string_nuke()
{
	strpool_t* pool = s_pool(1);
	strpool_term(pool);
}

}
