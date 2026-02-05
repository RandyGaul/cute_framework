/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// String manipulation and interning are now provided by ckit.h.
// This file contains only CF wrappers and cf_decode_UTF16 (CF-specific).

#include <cute_string.h>

// cf_sintern wraps ckit's implementation directly (avoid macro recursion).
const char* cf_sintern(const char* s)
{
	return ck_sintern(s);
}

const char* cf_sintern_range(const char* start, const char* end)
{
	return ck_sintern_range(start, end);
}

void cf_sinuke_intern_table(void)
{
	sintern_nuke();
}

// cf_decode_UTF8 is provided by ckit.h (compiled in cute_ckit.cpp).
// All invalid characters are encoded as the "replacement character" 0xFFFD.

const uint16_t* cf_decode_UTF16(const uint16_t* s, int* codepoint)
{
	int W1 = *s++;
	if (W1 < 0xD800 || W1 > 0xDFFF) {
		*codepoint = W1;
	} else if (W1 > 0xD800 && W1 < 0xDBFF) {
		int W2 = *s++;
		if (W2 > 0xDC00 && W2 < 0xDFFF) *codepoint = 0x10000 + (((W1 & 0x03FF) << 10) | (W2 & 0x03FF));
		else *codepoint = 0xFFFD;
	} else *codepoint = 0xFFFD;
	return s;
}
