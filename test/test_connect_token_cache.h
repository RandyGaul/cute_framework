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

#include <cute_alloc.h>

#include <internal/cute_protocol_internal.h>
using namespace cute;

CUTE_TEST_CASE(test_connect_token_cache, "Add tokens, overflow (eject oldest), ensure LRU correctness.");
int test_connect_token_cache()
{
	int capacity = 3;
	protocol::connect_token_cache_t cache;
	protocol::connect_token_cache_init(&cache, capacity, NULL);

	endpoint_t endpoint;
	CUTE_TEST_CHECK(endpoint_init(&endpoint, "[::]:5000"));

	uint8_t hmac_bytes_a[CUTE_CRYPTO_HEADER_BYTES];
	uint8_t hmac_bytes_b[CUTE_CRYPTO_HEADER_BYTES];
	uint8_t hmac_bytes_c[CUTE_CRYPTO_HEADER_BYTES];
	uint8_t hmac_bytes_d[CUTE_CRYPTO_HEADER_BYTES];
	uint8_t hmac_bytes_e[CUTE_CRYPTO_HEADER_BYTES];
	crypto_random_bytes(hmac_bytes_a, sizeof(hmac_bytes_a));
	crypto_random_bytes(hmac_bytes_b, sizeof(hmac_bytes_b));
	crypto_random_bytes(hmac_bytes_c, sizeof(hmac_bytes_c));
	crypto_random_bytes(hmac_bytes_d, sizeof(hmac_bytes_d));
	crypto_random_bytes(hmac_bytes_e, sizeof(hmac_bytes_e));

	protocol::connect_token_cache_add(&cache, hmac_bytes_a);
	CUTE_TEST_ASSERT(protocol::connect_token_cache_find(&cache, hmac_bytes_a));
	protocol::connect_token_cache_add(&cache, hmac_bytes_b);
	CUTE_TEST_ASSERT(protocol::connect_token_cache_find(&cache, hmac_bytes_b));
	protocol::connect_token_cache_add(&cache, hmac_bytes_c);
	CUTE_TEST_ASSERT(protocol::connect_token_cache_find(&cache, hmac_bytes_c));
	protocol::connect_token_cache_add(&cache, hmac_bytes_d);
	CUTE_TEST_ASSERT(protocol::connect_token_cache_find(&cache, hmac_bytes_d));

	CUTE_TEST_ASSERT(!protocol::connect_token_cache_find(&cache, hmac_bytes_a));
	CUTE_TEST_ASSERT(protocol::connect_token_cache_find(&cache, hmac_bytes_b));
	CUTE_TEST_ASSERT(protocol::connect_token_cache_find(&cache, hmac_bytes_c));
	CUTE_TEST_ASSERT(protocol::connect_token_cache_find(&cache, hmac_bytes_d));

	protocol::connect_token_cache_add(&cache, hmac_bytes_e);
	CUTE_TEST_ASSERT(protocol::connect_token_cache_find(&cache, hmac_bytes_e));

	CUTE_TEST_ASSERT(!protocol::connect_token_cache_find(&cache, hmac_bytes_a));
	CUTE_TEST_ASSERT(!protocol::connect_token_cache_find(&cache, hmac_bytes_b));
	CUTE_TEST_ASSERT(protocol::connect_token_cache_find(&cache, hmac_bytes_c));
	CUTE_TEST_ASSERT(protocol::connect_token_cache_find(&cache, hmac_bytes_d));
	CUTE_TEST_ASSERT(protocol::connect_token_cache_find(&cache, hmac_bytes_e));

	protocol::connect_token_cache_cleanup(&cache);

	return 0;
}
