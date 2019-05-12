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

#include <cute_kv.h>
using namespace cute;

struct thing_t
{
	int a;
	float b;
	char* str;
	int str_len;
};

// WORKING HERE
// Need to output this string then parse it.
/*
{
	a = 5,
	b = 10.300000,
	str = "Hello.",
	sub_thing = {
		a = 5,
		die_fucker = 5,
		interior_thing = {
			hi = 5,
			geez = "Hello.",
		},
	},
	x = 5,
	y = 10.300000,
	blob_data = "U29tZSBibG9iIGlucHV0LgA=",
	array_of_ints [8] {
		0, 1, 2, 3, 4, 5, 6, 7,
	},
	array_of_array_of_ints [2] {
		[3] {
			0, 1, 2
		},
		[3] {
			2, 5, 1
		},
	},
	array_of_objects [3] {
		{
			some_integer = 5,
			some_string = "Hello.",
		},
		{
			some_integer = 5,
			some_string = "Hello.",
		},
		{
			some_integer = 5,
			some_string = "Hello.",
		},
	},
},
*/

void do_serialize(kv_t* kv, thing_t* thing)
{
	kv_object_begin(kv);
	kv_key(kv, "a"); kv_val(kv, &thing->a);
	kv_key(kv, "b"); kv_val(kv, &thing->b);
	kv_key(kv, "str"); kv_val_string(kv, &thing->str, &thing->str_len);
		kv_key(kv, "sub_thing"); kv_object_begin(kv);
		kv_key(kv, "a"); kv_val(kv, &thing->a);
		kv_key(kv, "die_fucker"); kv_val(kv, &thing->a);
			kv_key(kv, "interior_thing");  kv_object_begin(kv);
			kv_key(kv, "hi"); kv_val(kv, &thing->a);
			kv_key(kv, "geez"); kv_val_string(kv, &thing->str, &thing->str_len);
			kv_object_end(kv);
		kv_object_end(kv);
	kv_key(kv, "x"); kv_val(kv, &thing->a);
	kv_key(kv, "y"); kv_val(kv, &thing->b);
	int blob_size = 17;
	kv_key(kv, "blob_data"); kv_val_blob(kv, "Some blob input.", &blob_size);
	int int_count = 8;
	kv_key(kv, "array_of_ints"); kv_array_begin(kv, &int_count);
		for (int i = 0; i < int_count; ++i) kv_val(kv, &i);
	kv_array_end(kv);
	int_count = 2;
	kv_key(kv, "array_of_array_of_ints"); kv_array_begin(kv, &int_count);
		int_count = 3;
		kv_array_begin(kv, &int_count);
		for (int i = 0; i < int_count; ++i)
		{
			kv_val(kv, &i);
		}
		kv_array_end(kv);
		kv_array_begin(kv, &int_count);
		for (int i = 0; i < int_count; ++i)
		{
			kv_val(kv, &i);
		}
		kv_array_end(kv);
	kv_array_end(kv);
	kv_key(kv, "array_of_objects"); kv_array_begin(kv, &int_count);
	for (int i = 0; i < int_count; ++i)
	{
		kv_object_begin(kv);
		kv_key(kv, "some_integer"); kv_val(kv, &thing->a);
		kv_key(kv, "some_string"); kv_val_string(kv, &thing->str, &thing->str_len);
		kv_object_end(kv);
	}
	kv_array_end(kv);
	kv_object_end(kv);
}

CUTE_TEST_CASE(test_kv_basic, "FUCKERS.");
int test_kv_basic()
{
	kv_t* kv = kv_make();

	char buffer[1024];
	kv_reset(kv, buffer, sizeof(buffer), CUTE_KV_MODE_WRITE);

	thing_t thing;
	thing.a = 5;
	thing.b = 10.3f;
	thing.str = "Hello.";
	thing.str_len = (int)CUTE_STRLEN(thing.str);

	do_serialize(kv, &thing);

	kv_print(kv);

	int size = kv_size_written(kv);

	kv_reset(kv, buffer, size, CUTE_KV_MODE_READ);

	kv_destroy(kv);

	return -1;
}
