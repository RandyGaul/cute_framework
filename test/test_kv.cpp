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
#include <cute_kv.h>
using namespace Cute;

struct thing_t
{
	int a = 5;
	float b = 10.3f;
	const char* str = "Hello.";
	struct
	{
		int a = 5;
		int die_pls = 5;
		struct
		{
			int hi = 5;
			const char* geez = "Hello.";
		} interior_thing;
	} sub_thing;
	float x = 5;
	float y = 10.3f;
	char blob_data[24] = "Some blob input.";
	int array_of_ints[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	int array_of_array_of_ints[2][3] = {
		{ 0, 1, 2 },
		{ 0, 1, 2 },
	};
	struct
	{
		int some_integer;
		const char* some_string;
	} array_of_objects[3] = {
		{ 3, "Hi." },
		{ 4, "Hi.." },
		{ 5, "Hi..." },
	};
};

bool do_serialize(CF_KeyValue* kv, thing_t* thing)
{
	size_t len;
	REQUIRE(cf_kv_key(kv, "a", NULL)); REQUIRE(cf_kv_val_int32(kv, &thing->a));
	REQUIRE(cf_kv_key(kv, "b", NULL)); REQUIRE(cf_kv_val_float(kv, &thing->b));
	REQUIRE(cf_kv_key(kv, "str", NULL)); len = 6; REQUIRE(cf_kv_val_string(kv, &thing->str, &len));
		REQUIRE(cf_kv_object_begin(kv, "sub_thing"));
		REQUIRE(cf_kv_key(kv, "a", NULL)); REQUIRE(cf_kv_val_int32(kv, &thing->sub_thing.a));
		REQUIRE(cf_kv_key(kv, "die_pls", NULL)); REQUIRE(cf_kv_val_int32(kv, &thing->sub_thing.die_pls));
			REQUIRE(cf_kv_object_begin(kv, "interior_thing"));
			REQUIRE(cf_kv_key(kv, "hi", NULL)); REQUIRE(cf_kv_val_int32(kv, &thing->sub_thing.interior_thing.hi));
			REQUIRE(cf_kv_key(kv, "geez", NULL)); len = 6; REQUIRE(cf_kv_val_string(kv, &thing->sub_thing.interior_thing.geez, &len));
			REQUIRE(cf_kv_object_end(kv));
		REQUIRE(cf_kv_object_end(kv));
	REQUIRE(cf_kv_key(kv, "x", NULL)); REQUIRE(cf_kv_val_float(kv, &thing->x));
	REQUIRE(cf_kv_key(kv, "y", NULL)); REQUIRE(cf_kv_val_float(kv, &thing->y));
	size_t blob_data_lem = CF_STRLEN(thing->blob_data) + 1;
	REQUIRE(cf_kv_key(kv, "blob_data", NULL)); REQUIRE(cf_kv_val_blob(kv, thing->blob_data, sizeof(thing->blob_data), &blob_data_lem));
	int int_count = 8;
	REQUIRE(cf_kv_array_begin(kv, &int_count, "array_of_ints"));
		for (int i = 0; i < int_count; ++i) REQUIRE(cf_kv_val_int32(kv, thing->array_of_ints + i));
	REQUIRE(cf_kv_array_end(kv));
	int_count = 2;
	REQUIRE(cf_kv_array_begin(kv, &int_count, "array_of_array_of_ints"));
		int_count = 3;
		REQUIRE(cf_kv_array_begin(kv, &int_count, NULL));
		for (int i = 0; i < int_count; ++i)
		{
			REQUIRE(cf_kv_val_int32(kv, thing->array_of_array_of_ints[0] + i));
		}
		REQUIRE(cf_kv_array_end(kv));
		REQUIRE(cf_kv_array_begin(kv, &int_count, NULL));
		for (int i = 0; i < int_count; ++i)
		{
			REQUIRE(cf_kv_val_int32(kv, thing->array_of_array_of_ints[1] + i));
		}
		REQUIRE(cf_kv_array_end(kv));
	REQUIRE(cf_kv_array_end(kv));
	REQUIRE(cf_kv_array_begin(kv, &int_count, "array_of_objects"));
	for (int i = 0; i < int_count; ++i)
	{
		REQUIRE(cf_kv_object_begin(kv, NULL));
		REQUIRE(cf_kv_key(kv, "some_integer", NULL)); REQUIRE(cf_kv_val_int32(kv, &thing->array_of_objects[i].some_integer));
		if (i == 0) len = 3;
		else if (i == 1) len = 4;
		else if (i == 2) len = 5;
		REQUIRE(cf_kv_key(kv, "some_string", NULL)); REQUIRE(cf_kv_val_string(kv, &thing->array_of_objects[i].some_string, &len));
		REQUIRE(cf_kv_object_end(kv));
	}
	REQUIRE(cf_kv_array_end(kv));
	return true;
}

/* Fairly comprehensive test for basic kv to and from buffer. */
TEST_CASE(test_kv_basic)
{
	CF_KeyValue* kv0 = cf_kv_write();

	thing_t thing;
	thing.a = 5;
	thing.b = 10.3f;

	REQUIRE(do_serialize(kv0, &thing));

	const char* expected =
	"a = 5,\n"
	"b = 10.300000,\n"
	"str = \"Hello.\",\n"
	"sub_thing = {\n"
	"	a = 5,\n"
	"	die_pls = 5,\n"
	"	interior_thing = {\n"
	"		hi = 5,\n"
	"		geez = \"Hello.\",\n"
	"	},\n"
	"},\n"
	"x = 5.000000,\n"
	"y = 10.300000,\n"
	"blob_data = \"U29tZSBibG9iIGlucHV0LgA=\",\n"
	"array_of_ints = [8] {\n"
	"	0, 1, 2, 3, 4, 5, 6, 7,\n"
	"},\n"
	"array_of_array_of_ints = [2] {\n"
	"	[3] {\n"
	"		0, 1, 2,\n"
	"	},\n"
	"	[3] {\n"
	"		0, 1, 2,\n"
	"	},\n"
	"},\n"
	"array_of_objects = [3] {\n"
	"	{\n"
	"		some_integer = 3,\n"
	"		some_string = \"Hi.\",\n"
	"	},\n"
	"	{\n"
	"		some_integer = 4,\n"
	"		some_string = \"Hi..\",\n"
	"	},\n"
	"	{\n"
	"		some_integer = 5,\n"
	"		some_string = \"Hi...\",\n"
	"	},\n"
	"},\n"
	;

	size_t size = cf_kv_buffer_size(kv0);
	const char* buffer = cf_kv_buffer(kv0);
	REQUIRE(!CF_STRNCMP(buffer, expected, size));

	CF_KeyValue* kv1 = cf_kv_read(buffer, size, NULL);
	REQUIRE(kv1);

	CF_MEMSET(&thing, 0, sizeof(thing_t));
	REQUIRE(do_serialize(kv1, &thing));

	cf_kv_destroy(kv0);
	cf_kv_destroy(kv1);

	return true;
}

#include <string>

CF_INLINE bool cf_kv_val(CF_KeyValue* kv, std::string* val)
{
	const char* ptr = val->data();
	size_t len = val->length();
	if (!cf_kv_val_string(kv, &ptr, &len)) return false;
	val->assign(ptr, len);
	return true;
}

/* Testing kv utility for c-string to disk, std::string from disk. */
TEST_CASE(test_kv_std_string_to_disk)
{
	std::string s0;
	const char* s1 = "Alice in Wonderland.";
	size_t s1_len = CF_STRLEN(s1);

	CF_KeyValue* kv0 = cf_kv_write();
	cf_kv_key(kv0, "book_title", NULL);
	cf_kv_val_string(kv0, &s1, &s1_len);

	REQUIRE(!cf_is_error(cf_kv_last_error(kv0)));
	size_t size = cf_kv_buffer_size(kv0);
	CF_KeyValue* kv1 = cf_kv_read(cf_kv_buffer(kv0), size, NULL);
	REQUIRE(kv1);

	cf_kv_key(kv1, "book_title", NULL);
	cf_kv_val(kv1, &s0);

	REQUIRE(!cf_is_error(cf_kv_last_error(kv1)));
	REQUIRE(s0.length() == s1_len);
	REQUIRE(!CF_STRNCMP(s0.data(), s1, s1_len));

	cf_kv_destroy(kv0);
	cf_kv_destroy(kv1);

	return true;
}

/* Testing kv utility for std::string to disk, c-string from disk. */
TEST_CASE(test_kv_std_string_from_disk)
{
	// std::string from disk, c-string to disk
	const char* s0 = NULL;
	size_t s0_len = 0;
	std::string s1 = "Alice in Wonderland.";
	size_t s1_len = s1.length();

	CF_KeyValue* kv0 = cf_kv_write();
	cf_kv_key(kv0, "book_title", NULL);
	cf_kv_val(kv0, &s1);

	REQUIRE(!cf_is_error(cf_kv_last_error(kv0)));
	size_t size = cf_kv_buffer_size(kv0);
	CF_KeyValue* kv1 = cf_kv_read(cf_kv_buffer(kv0), size, NULL);
	REQUIRE(kv1);

	cf_kv_key(kv1, "book_title", NULL);
	cf_kv_val_string(kv1, &s0, &s0_len);

	REQUIRE(!cf_is_error(cf_kv_last_error(kv1)));
	REQUIRE((int)s1.length() == s0_len);
	REQUIRE(!CF_STRNCMP(s1.data(), s0, s0_len));

	cf_kv_destroy(kv0);
	cf_kv_destroy(kv1);

	return true;
}

#include <vector>

template <typename T>
CF_INLINE bool cf_kv_val(CF_KeyValue* kv, std::vector<T>* val, const char* key = NULL)
{
	int count = (int)val->size();
	if (cf_kv_array_begin(kv, &count, key))
	{
		val->resize(count);
		for (int i = 0; i < count; ++i) {
			cf_kv_val_int32(kv, &(*val)[i]);
		}
		cf_kv_array_end(kv);
		return !cf_is_error(cf_kv_last_error(kv));
	}
	return false;
}

/* Testing kv utility for std::vector support. */
TEST_CASE(test_kv_std_vector)
{
	CF_KeyValue* kv0 = cf_kv_write();

	std::vector<int> v;
	v.push_back(10);
	v.push_back(-3);
	v.push_back(17);
	v.push_back(5);
	v.push_back(0);
	v.push_back(100);
	v.push_back(6);
	v.push_back(-2);

	cf_kv_val(kv0, &v, "vector_of_ints");

	REQUIRE(!cf_is_error(cf_kv_last_error(kv0)));
	size_t size = cf_kv_buffer_size(kv0);
	CF_KeyValue* kv1 = cf_kv_read(cf_kv_buffer(kv0), size, NULL);
	REQUIRE(kv1);

	v.clear();
	cf_kv_val(kv1, &v, "vector_of_ints");

	REQUIRE(v.size() == 8);
	REQUIRE(v[0] == 10);
	REQUIRE(v[1] == -3);
	REQUIRE(v[2] == 17);
	REQUIRE(v[3] == 5);
	REQUIRE(v[4] == 0);
	REQUIRE(v[5] == 100);
	REQUIRE(v[6] == 6);
	REQUIRE(v[7] == -2);

	cf_kv_destroy(kv0);
	cf_kv_destroy(kv1);

	return true;
}

TEST_SUITE(test_kv)
{
	RUN_TEST_CASE(test_kv_basic);
	RUN_TEST_CASE(test_kv_std_string_to_disk);
	RUN_TEST_CASE(test_kv_std_string_from_disk);
	RUN_TEST_CASE(test_kv_std_vector);
}
