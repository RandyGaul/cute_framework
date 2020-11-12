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

#include <cute_string_utils.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>

#include <stdio.h>
#include <inttypes.h>

namespace cute
{

static size_t s_temp_str_size;
static char* s_temp_str;

static char* s_temp(size_t size)
{
	if (s_temp_str_size < size + 1) {
		CUTE_FREE(s_temp_str, NULL);
		s_temp_str_size = size + 1;
		s_temp_str = (char*)CUTE_ALLOC(size + 1, );
	}
	return s_temp_str;
}

void string_utils_cleanup_static_memory()
{
	CUTE_FREE(s_temp_str, NULL);
	s_temp_str = NULL;
}

//--------------------------------------------------------------------------------------------------

string_t operator+(const string_t& a, const string_t& b)
{
	size_t len_a = a.len();
	size_t len_b = b.len();
	char* temp = s_temp(len_a + len_b);
	CUTE_MEMCPY(temp, a.c_str(), len_a);
	CUTE_MEMCPY(temp + len_a, b.c_str(), len_b);
	temp[len_a + len_b] = 0;
	return string_t(temp);
}

int to_int(const string_t& x)
{
	const char* s = x.c_str();
	return (int)CUTE_STRTOLL(s, NULL, 10);
}

float to_float(const string_t& x)
{
	const char* s = x.c_str();
	return strtof(s, NULL);
}

string_t format(string_t fmt, int n, ...)
{
	va_list args;
	va_start(args, n);
	char* temp = s_temp(256);

	#ifdef _WIN32
		int size = _vscprintf(fmt.c_str(), args) + 1;
	#else
		int size = vsnprintf(temp, 0, fmt.c_str(), args) + 1;
	#endif

	temp = s_temp(size);

	#ifdef _WIN32
		_vsnprintf(temp, size, fmt.c_str(), args);
	#else
		vsnprintf(temp, size, fmt.c_str(), args);
	#endif

	va_end(args);
	return string_t(temp);
}

string_t to_string(int x)
{
	const char* fmt = "%d";
	char* temp = s_temp(256);

	#ifdef _WIN32
		int size = _scprintf(fmt, x) + 1;
	#else
		int size = snprintf(temp, 0, fmt, x) + 1;
	#endif

	temp = s_temp(size);

	snprintf(temp, size, fmt, x);

	return string_t(temp);
}

string_t to_string(uint64_t x)
{
	const char* fmt = "%" PRIu64;
	char* temp = s_temp(256);

	#ifdef _WIN32
		int size = _scprintf(fmt, x) + 1;
	#else
		int size = snprintf(temp, 0, fmt, x) + 1;
	#endif

	temp = s_temp(size);

	snprintf(temp, size, fmt, x);

	return string_t(temp);
}

string_t to_string(float x)
{
	const char* fmt = "%f";
	char* temp = s_temp(256);

	#ifdef _WIN32
		int size = _scprintf(fmt, x) + 1;
	#else
		int size = snprintf(temp, 0, fmt, x) + 1;
	#endif

	temp = s_temp(size);

	snprintf(temp, size, fmt, x);

	return string_t(temp);
}

array<char> to_array(const char* s)
{
	array<char> result;
	char c;
	while ((c = *s++)) result.add(c);
	result.add(0);
	return result;
}

array<char> to_array(const char* s, size_t sz)
{
	array<char> result;
	while (sz--) result.add(*s++);
	result.add(0);
	return result;
}

}
