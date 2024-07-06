/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
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
