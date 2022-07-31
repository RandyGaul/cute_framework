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
#include <cute_strpool.h>

#include <internal/cute_app_internal.h>


//--------------------------------------------------------------------------------------------------
// Global `string` pool instance.

static void* cf_s_mem_ctx;
static bool cf_s_pool_init;
static cf_strpool_t* cf_s_pool_instance;

static cf_strpool_t* cf_s_pool(int nuke = 0)
{
	cf_strpool_t* instance = cf_s_pool_instance;
	if (nuke) {
		if (cf_s_pool_init) {
			cf_destroy_strpool(instance);
			cf_s_pool_instance = NULL;
		}
		cf_s_pool_init = 0;
		return NULL;
	} else {
		if (!cf_s_pool_init) {
			cf_s_pool_instance = instance = cf_make_strpool(NULL);
			cf_s_pool_init = 1;
		}
		return instance;
	}
}

static void cf_s_incref(cf_strpool_id id)
{
	if (id.val) {
		cf_strpool_incref(cf_s_pool(), id);
	}
}

static void cf_s_decref(cf_strpool_id id)
{
	if (id.val) {
		cf_strpool_t* pool = cf_s_pool();
		if (cf_strpool_decref(pool, id) <= 0) {
			cf_strpool_discard(pool, id);
		}
	}
}

//--------------------------------------------------------------------------------------------------

cf_string_t::cf_string_t()
	: id({ 0 })
{
}

cf_string_t::cf_string_t(char* str)
{
	int len = (int)CUTE_STRLEN(str);
	if (len) {
		id = cf_strpool_inject_len(cf_s_pool(), str, len);
		cf_s_incref(id);
	} else id.val = 0;
}

cf_string_t::cf_string_t(const char* str)
{
	int len = str ? (int)CUTE_STRLEN(str) : 0;
	if (len) {
		id = cf_strpool_inject_len(cf_s_pool(), str, len);
		cf_s_incref(id);
	} else id.val = 0;
}

cf_string_t::cf_string_t(const char* begin, const char* end)
{
	int len = begin ? (end ? (int)(end - begin) : (int)CUTE_STRLEN(begin)) : 0;
	if (len) {
		id = cf_strpool_inject_len(cf_s_pool(), begin, len);
		cf_s_incref(id);
	} else id.val = 0;
}

cf_string_t::cf_string_t(const cf_string_t& other)
	: id(other.id)
{
	cf_s_incref(id);
}

cf_string_t::cf_string_t(cf_strpool_id id)
	: id(id)
{
}

cf_string_t::~cf_string_t()
{
	cf_s_decref(id);
}

size_t cf_string_t::len() const
{
	return cf_strpool_length(cf_s_pool(), id);
}

const char* cf_string_t::c_str() const
{
	return cf_strpool_cstr(cf_s_pool(), id);
}

cf_string_t& cf_string_t::operator=(const cf_string_t& rhs)
{
	cf_s_incref(rhs.id);
	cf_s_decref(id);
	id = rhs.id;
	return *this;
}

bool cf_string_t::operator==(const cf_string_t& rhs) const
{
	return id.val == rhs.id.val;
}

bool cf_string_t::operator!=(const cf_string_t& rhs) const
{
	return id.val != rhs.id.val;
}

char cf_string_t::operator[](const int i) const
{
	CUTE_ASSERT(i >= 0 && i < len());
	return c_str()[i];
}

void cf_string_t::incref()
{
	cf_s_incref(id);
}

void cf_string_t::decref()
{
	cf_s_decref(id);
}

bool cf_string_t::is_valid() const
{
	return cf_strpool_isvalid(cf_s_pool(), id);
}

void cf_string_defrag_static_pool()
{
	cf_strpool_defrag(cf_s_pool());
}

void cf_string_nuke_static_pool()
{
	cf_s_pool(1);
}
