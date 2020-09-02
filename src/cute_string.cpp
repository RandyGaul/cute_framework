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
		if (s_pool_init) {
			strpool_term(instance);
		}
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

static void s_incref(strpool_t* pool, uint64_t id)
{
	pool = pool ? pool : s_pool();
	if (id) {
		strpool_incref(pool, id);
	}
}

static void s_decref(strpool_t* pool, uint64_t id)
{
	pool = pool ? pool : s_pool();
	if (id) {
		if (strpool_decref(pool, id) <= 0) {
			strpool_discard(pool, id);
		}
	}
}

//--------------------------------------------------------------------------------------------------

string_t::string_t(strpool_t* pool)
	: id(0)
	, pool(pool)
{
}

string_t::string_t(char* str, strpool_t* pool)
{
	int len = (int)CUTE_STRLEN(str);
	if (len) {
		id = strpool_inject(pool = pool ? pool : s_pool(), str, len);
		s_incref(pool, id);
	} else id = 0;
	this->pool = pool;
}

string_t::string_t(const char* str, strpool_t* pool)
{
	int len = str ? (int)CUTE_STRLEN(str) : 0;
	if (len) {
		id = strpool_inject(pool = pool ? pool : s_pool(), str, len);
		s_incref(pool, id);
	} else id = 0;
	this->pool = pool;
}

string_t::string_t(const char* begin, const char* end, strpool_t* pool)
{
	int len = begin ? (end ? (int)(end - begin) : (int)CUTE_STRLEN(begin)) : 0;
	if (len) {
		id = strpool_inject(pool = pool ? pool : s_pool(), begin, len);
		s_incref(pool, id);
	} else id = 0;
	this->pool = pool;
}

string_t::string_t(const string_t& other, strpool_t* pool)
	: id(other.id)
	, pool(pool ? pool : other.pool)
{
	s_incref(pool, id);
}

string_t::~string_t()
{
	s_decref(pool, id);
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
	s_incref(rhs.pool, rhs.id);
	s_decref(pool, id);
	id = rhs.id;
	pool = rhs.pool;
	return *this;
}

int string_t::operator==(const string_t& rhs) const
{
	return id == rhs.id && pool == rhs.pool;
}

int string_t::operator!=(const string_t& rhs) const
{
	return id != rhs.id && pool != rhs.pool;
}

char string_t::operator[](const int i) const
{
	CUTE_ASSERT(i >= 0 && i < len());
	return c_str()[i];
}

void string_t::incref()
{
	s_incref(pool, id);
}

void string_t::decref()
{
	s_decref(pool, id);
}

bool string_t::is_valid()
{
	return strpool_isvalid(pool ? pool : s_pool(), id);
}

void string_defrag_static_pool()
{
	strpool_defrag(s_pool());
}

void string_nuke_static_pool()
{
	s_pool(1);
}

}
