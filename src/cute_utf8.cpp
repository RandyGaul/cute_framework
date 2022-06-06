/*
	Cute Framework
	Copyright (C) 2021 Randy Gaul https://randygaul.net

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

#include <cute_utf8.h>

#define CUTE_UTF_IMPLEMENTATION
#include <cute/cute_utf.h>

const char* cf_decode8(const char* text, int* cp)
{
	return cu_decode8(text, cp);
}

char* cf_encode8(char *text, int cp)
{
	return cu_encode8(text, cp);
}

int cf_codepoint8_size(int cp)
{
	return cu_codepoint8_size(cp);
}

const wchar_t* cf_decode16(const wchar_t* text, int* cp)
{
	return cu_decode16(text, cp);
}

wchar_t* cf_encode16(wchar_t* text, int cp)
{
	return cu_encode16(text, cp);
}

int cf_codepoint16_size(int cp)
{
	return cu_codepoint16_size(cp);
}

void cf_widen(const char* in, wchar_t* out)
{
	cu_widen(in, out);
}

void cf_widen(const char* in, int in_len, wchar_t* out)
{
	cu_widen(in, in_len, out);
}

void cf_widen(const char* in, wchar_t* out, int out_len)
{
	cu_widen(in, out, out_len);
}

void cf_widen(const char* in, int in_len, wchar_t* out, int out_len)
{
	cu_widen(in, in_len, out, out_len);
}

void cf_shorten(const wchar_t* in, char* out)
{
	cu_shorten(in, out);
}

void cf_shorten(const wchar_t* in, int in_len, char* out)
{
	cu_shorten(in, in_len, out);
}

void cf_shorten(const wchar_t* in, char* out, int out_len)
{
	cu_shorten(in, out, out_len);
}

void cf_shorten(const wchar_t* in, int in_len, char* out, int out_len)
{
	cu_shorten(in, in_len, out, out_len);
}
