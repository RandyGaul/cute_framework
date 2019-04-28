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

#include <cute_base64.h>
#include <cute_c_runtime.h>

namespace cute
{

static const uint8_t s_byte_to_base64[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

error_t base64_encode(void* dst, int dst_size, const void* src, int src_size)
{
	int out_size = CUTE_BASE64_SIZE(src_size);
	if (dst_size < out_size) return error_failure("base64 : dst buffer too small to place encoded output.");

	int triplets = (src_size) / 3;
	int pads = (src_size) % 3 ? 3 - (src_size) % 3 : 0;
	int index = 0;

	const uint8_t* in = (const uint8_t*)src;
	uint8_t* out = (uint8_t*)dst;

	while (triplets--)
	{
		uint32_t bits = ((uint32_t)in[0]) << 16 | ((uint32_t)in[1]) << 8 | ((uint32_t)in[2]);
		uint32_t a = (bits & 0xFC0000) >> 18;
		uint32_t b = (bits & 0x3F000) >> 12;
		uint32_t c = (bits & 0xFC0) >> 6;
		uint32_t d = (bits & 0x3F);
		in += 3;
		CUTE_ASSERT(a < 64);
		CUTE_ASSERT(b < 64);
		CUTE_ASSERT(c < 64);
		CUTE_ASSERT(d < 64);
		*out++ = s_byte_to_base64[a];
		*out++ = s_byte_to_base64[b];
		*out++ = s_byte_to_base64[c];
		*out++ = s_byte_to_base64[d];
	}

	switch (pads)
	{
	case 1:
	{
		uint32_t bits = ((uint32_t)in[0]) << 8 | ((uint32_t)in[1]);
		uint32_t a = (bits & 0xFC00) >> 10;
		uint32_t b = (bits & 0x3F0) >> 4;
		uint32_t c = (bits & 0xF) << 2;
		CUTE_ASSERT(a < 64);
		CUTE_ASSERT(b < 64);
		CUTE_ASSERT(c < 64);
		*out++ = s_byte_to_base64[a];
		*out++ = s_byte_to_base64[b];
		*out++ = s_byte_to_base64[c];
		in += 2;
	}	break;

	case 2:
		uint32_t bits = ((uint32_t)in[0]);
		uint32_t a = (bits & 0xFC) >> 2;
		uint32_t b = (bits & 0x3) << 4;
		CUTE_ASSERT(a < 64);
		CUTE_ASSERT(b < 64);
		*out++ = s_byte_to_base64[a];
		*out++ = s_byte_to_base64[b];
		in += 1;
		break;
	}

	while (pads--)
	{
		*out++ = '=';
	}

	CUTE_ASSERT((int)(out - (uint8_t*)dst) == out_size);

	return error_success();
}

error_t base64_decode(void* dst, int dst_size, const void* src, int src_size)
{
	return error_failure("not implemented");
}

}
