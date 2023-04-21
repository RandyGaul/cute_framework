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

#include "test_harness.h"

#include <cute_c_runtime.h>
#include <cute_base64.h>
using namespace Cute;

/* Test vectors from RFC 4648. */
TEST_CASE(test_base64_encode)
{
	uint8_t buffer[256];

	// Test vectors from: https://tools.ietf.org/html/rfc4648#section-10

	CHECK(cf_is_error(cf_base64_encode(buffer, 256, "", 0)));

	CHECK(cf_is_error(cf_base64_encode(buffer, 256, "f", 1)));
	REQUIRE(!CF_MEMCMP(buffer, "Zg==", 4));

	CHECK(cf_is_error(cf_base64_encode(buffer, 256, "fo", 2)));
	REQUIRE(!CF_MEMCMP(buffer, "Zm8=", 4));

	CHECK(cf_is_error(cf_base64_encode(buffer, 256, "foo", 3)));
	REQUIRE(!CF_MEMCMP(buffer, "Zm9v", 4));

	CHECK(cf_is_error(cf_base64_encode(buffer, 256, "foob", 4)));
	REQUIRE(!CF_MEMCMP(buffer, "Zm9vYg==", 8));

	CHECK(cf_is_error(cf_base64_encode(buffer, 256, "fooba", 5)));
	REQUIRE(!CF_MEMCMP(buffer, "Zm9vYmE=", 8));

	CHECK(cf_is_error(cf_base64_encode(buffer, 256, "foobar", 6)));
	REQUIRE(!CF_MEMCMP(buffer, "Zm9vYmFy", 8));
	
	CHECK(cf_is_error(cf_base64_decode(buffer, 256, "", 0)));

	CHECK(cf_is_error(cf_base64_decode(buffer, 256, "Zg==", 4)));
	REQUIRE(!CF_MEMCMP(buffer, "f", 1));

	CHECK(cf_is_error(cf_base64_decode(buffer, 256, "Zm8=", 4)));
	REQUIRE(!CF_MEMCMP(buffer, "fo", 2));

	CHECK(cf_is_error(cf_base64_decode(buffer, 256, "Zm9v", 4)));
	REQUIRE(!CF_MEMCMP(buffer, "foo", 3));

	CHECK(cf_is_error(cf_base64_decode(buffer, 256, "Zm9vYg==", 8)));
	REQUIRE(!CF_MEMCMP(buffer, "foob", 4));

	CHECK(cf_is_error(cf_base64_decode(buffer, 256, "Zm9vYmE=", 8)));
	REQUIRE(!CF_MEMCMP(buffer, "fooba", 5));

	CHECK(cf_is_error(cf_base64_decode(buffer, 256, "Zm9vYmFy", 8)));
	REQUIRE(!CF_MEMCMP(buffer, "foobar", 6));

	// Assert failure on some bad inputs.
	REQUIRE(cf_is_error(cf_base64_decode(buffer, 256, "f===", 4)));
	REQUIRE(cf_is_error(cf_base64_decode(buffer, 256, "foo~", 4)));
	REQUIRE(cf_is_error(cf_base64_decode(buffer, 256, "foo", 3)));
	REQUIRE(cf_is_error(cf_base64_decode(buffer, 256, "\\!@$", 4)));

	CHECK(cf_is_error(cf_base64_decode(buffer, 256, "zzz=", 4)));

	return true;
}

TEST_SUITE(test_base64)
{
	RUN_TEST_CASE(test_base64_encode);
}
