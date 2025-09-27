/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "cute_guid.h"
#include "cute_networking.h"

#ifdef CF_EMSCRIPTEN
#include <stddef.h>

// JS helper: fills [ptr, ptr+len) with crypto-secure bytes.
// No return value.
EM_JS(void, cf_js_fill_random, (void* ptr, int len), {
	if (typeof crypto !== 'undefined' && typeof crypto.getRandomValues === 'function') {
		const view = new Uint8Array(Module.HEAPU8.buffer, ptr, len);
		crypto.getRandomValues(view);
	} else {
		// Fallback: fill with zeros if crypto is unavailable
		const view = new Uint8Array(Module.HEAPU8.buffer, ptr, len);
		view.fill(0);
	}
});

void cf_crypto_random_bytes(void* dst, size_t len)
{
	if (!dst || !len) return;
	cf_js_fill_random(dst, (int)len);
}
#endif

CF_Guid cf_make_guid()
{
	CF_Guid guid;
	cf_crypto_random_bytes(guid.data, sizeof(guid.data));
	return guid;
}
