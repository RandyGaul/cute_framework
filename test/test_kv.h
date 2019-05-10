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

void do_serialize(kv_t* kv, thing_t* thing)
{
	kv_object_begin(kv, NULL, "thing_t");
	kv_field(kv, "a", &thing->a);
	kv_field(kv, "b", &thing->b);
	kv_field_str(kv, "str", &thing->str, &thing->str_len);
		kv_object_begin(kv, "sub_thing", "nested_thing_t");
		kv_field(kv, "a", &thing->a);
		kv_field(kv, "die_fucker", &thing->a);
			kv_object_begin(kv, "interior_thing", "final_nest_t");
			kv_field(kv, "hi", &thing->a);
			kv_field_str(kv, "geez", &thing->str, &thing->str_len);
			kv_object_end(kv);
		kv_object_end(kv);
	kv_field(kv, "x", &thing->a);
	kv_field(kv, "y", &thing->b);
	int blob_size = 17;
	kv_field_blob(kv, "blob_data", "Some blob input.", &blob_size);
	int int_count = 8;
	kv_field_array_begin(kv, "array_of_ints", &int_count);
		for (int i = 0; i < int_count; ++i) kv_field(kv, NULL, &i);
	kv_field_array_end(kv);
	int_count = 3;
	kv_field_array_begin(kv, "array_of_objects", &int_count, "array_object_t");
		for (int i = 0; i < int_count; ++i)
		{
			kv_object_begin(kv, NULL, NULL);
			kv_field(kv, "some integer.", &thing->a);
			kv_field_str(kv, "some string", &thing->str, &thing->str_len);
			kv_object_end(kv);
		}
	kv_field_array_end(kv);
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
