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

#include <cute_datalibrary.h>
using namespace cute;

#include <dl/dl.h>
#include <dl/dl_typelib.h>
#include <dl/dl_txt.h>

#define CUTE_TEST_STRINGIFY(...) #__VA_ARGS__

static error_t s_write_buffer_to_file(const char* path, const void* data, int size)
{
	FILE* fp = fopen(path, "wb");
	if (!fp) return error_failure("Unable to open file.");
	fwrite(data, (size_t)size, 1, fp);
	fclose(fp);
	return error_success();
}

CUTE_TEST_CASE(test_datalibrary_basic, "Run type compiler, store instance, load instance.");
int test_datalibrary_basic()
{
	const char* typelib = CUTE_TEST_STRINGIFY({
		"types" : {
			"data_t" : {
				"members" : [
					{ "name" : "id", "type" : "string" },
					{ "name" : "x", "type" : "fp32" },
					{ "name" : "y", "type" : "fp32" },
					{ "name" : "data", "type" : "uint32[]" }
				]
			}
		}
	});

	s_write_buffer_to_file("test_data_t.tld", typelib, (int)CUTE_STRLEN(typelib));

	struct data_t
	{
		const char* id = "Data Identifier.";
		float x = 10.0f;
		float y = 15.0f;
		struct
		{
			int* ptr;
			int count;
		} array;
	};

	int numbers[] = {
		7, 4, 3
	};

	compile_typelibs(".", ".");

	datalibrary_t* dl = datalibrary_make();
	CUTE_TEST_CHECK_POINTER(dl);

	datalibrary_load_typelib_binary(dl, ".");

	data_t data;
	data.array.ptr = numbers;
	data.array.count = sizeof(numbers) / sizeof(numbers[0]);

	char buffer[1024];

	int size;
	error_t err = datalibrary_store_instance(dl, "data_t", &data, buffer, sizeof(buffer), &size);
	if (err.is_error()) return -1;

	char loaded_data[1024];

	int consumed;
	err = datalibrary_load_instance(dl, "data_t", loaded_data, sizeof(loaded_data), buffer, size, &consumed);
	if (err.is_error()) return -1;
	if (size != consumed) return -1;

	data_t* data_ptr = (data_t*)loaded_data;

	CUTE_TEST_ASSERT(!CUTE_STRCMP(data.id, data_ptr->id));
	CUTE_TEST_ASSERT(data.x == data_ptr->x);
	CUTE_TEST_ASSERT(data.y == data_ptr->y);
	CUTE_TEST_ASSERT(data.array.count == data_ptr->array.count);
	for (int i = 0; i < data.array.count; ++i) {
		CUTE_TEST_ASSERT(data.array.ptr[i] == data_ptr->array.ptr[i]);
	}

	datalibrary_destroy(dl);

	remove("test_data_t.tld");
	remove("typelib_binary.bin");

	return 0;
}

#undef CUTE_TEST_STRINGIFY
