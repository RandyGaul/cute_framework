/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#include <cute.h>

using namespace Cute;

TEST_CASE(test_json_basic)
{
	CF_JDoc doc = cf_make_json(NULL, 0);
	CF_JVal root = cf_json_object(doc);
	cf_json_set_root(doc, root);

	cf_json_object_add_float(doc, root, "x", 1.0f);
	cf_json_object_add_float(doc, root, "y", 2.0f);
	cf_json_object_add_float(doc, root, "z", 3.0f);

	char* s0 = cf_json_to_string(doc);
	const char* s1 = 
		"{\n"
		"\t\"x\": 1.0,\n"
		"\t\"y\": 2.0,\n"
		"\t\"z\": 3.0\n"
		"}"
	;

	REQUIRE(!CF_STRCMP(s0, s1));
	sfree(s0);
	cf_destroy_json(doc);

	return true;
}

TEST_CASE(test_json_nested_objects)
{
	const char* s = 
		"{\n"
		"\t\"pos\": {\n"
		"\t\t\"x\": 10.0,\n"
		"\t\t\"y\": -5.0,\n"
		"\t},\n"
		"\t\"normal\": {\n"
		"\t\t\"x\": 1.0,\n"
		"\t\t\"y\": 0.0,\n"
		"\t},\n"
		"}"
	;
	CF_JDoc doc = cf_make_json(s, CF_STRLEN(s));
	CF_JVal root = cf_json_get_root(doc);

	CF_JIter iter = cf_json_iter(root);
	auto read_v2 = [&](const char* key) -> v2 {
		CF_JVal val = cf_json_iter_next_by_name(&iter, key);
		CF_JIter i = cf_json_iter(val);
		float x = cf_json_get_float(cf_json_iter_next_by_name(&i, "x"));
		float y = cf_json_get_float(cf_json_iter_next_by_name(&i, "y"));
		return V2(x,y);
	};

	v2 pos = read_v2("pos");
	v2 normal = read_v2("normal");
	REQUIRE(pos.x == 10.0f);
	REQUIRE(pos.y == -5.0f);
	REQUIRE(normal.x == 1.0f);
	REQUIRE(normal.y == 0.0f);

	cf_destroy_json(doc);

	return true;
}

TEST_CASE(test_json_array)
{
	CF_JDoc doc = cf_make_json(NULL, 0);
	CF_JVal root = cf_json_object(doc);
	cf_json_set_root(doc, root);

	CF_JVal words = cf_json_array(doc);
	cf_json_array_add_string(doc, words, "hello");
	cf_json_array_add_string(doc, words, "goodbye");
	cf_json_array_add_string(doc, words, "saturday");
	cf_json_object_add(doc, root, "words", words);

	for (CF_JIter iter = cf_json_iter(words); !cf_json_iter_done(iter); iter = cf_json_iter_next(iter)) {
		const char* val = cf_json_get_string(cf_json_iter_val(iter));
		if (iter.index == 0) {
			REQUIRE(!CF_STRCMP(val, "hello"));
		} else if (iter.index == 1) {
			REQUIRE(!CF_STRCMP(val, "goodbye"));
		} else {
			REQUIRE(!CF_STRCMP(val, "saturday"));
		}
	}

	char* s0 = cf_json_to_string(doc);
	const char* s1 =
		"{\n"
		"\t\"words\": [\n"
		"\t\t\"hello\",\n"
		"\t\t\"goodbye\",\n"
		"\t\t\"saturday\"\n"
		"\t]\n"
		"}"
	;
	REQUIRE(!CF_STRCMP(s0, s1));
	sfree(s0);
	cf_destroy_json(doc);

	return true;
}

TEST_CASE(test_json_iterate_object)
{
	CF_JDoc doc = cf_make_json(NULL, 0);
	CF_JVal root = cf_json_object(doc);
	cf_json_set_root(doc, root);

	cf_json_object_add_float(doc, root, "x", 1.0f);
	cf_json_object_add_float(doc, root, "y", 2.0f);
	cf_json_object_add_float(doc, root, "z", 3.0f);

	for (CF_JIter iter = cf_json_iter(root); !cf_json_iter_done(iter); iter = cf_json_iter_next(iter)) {
		const char* key = cf_json_iter_key(iter);
		float val = cf_json_get_float(cf_json_iter_val(iter));
		if (iter.index == 0) {
			REQUIRE(!CF_STRCMP(key, "x"));
			REQUIRE(val == 1.0f);
		} else if (iter.index == 1) {
			REQUIRE(!CF_STRCMP(key, "y"));
			REQUIRE(val == 2.0f);
		} else {
			REQUIRE(!CF_STRCMP(key, "z"));
			REQUIRE(val == 3.0f);
		}
	}

	cf_destroy_json(doc);

	return true;
}

TEST_SUITE(test_json)
{
	RUN_TEST_CASE(test_json_basic);
	RUN_TEST_CASE(test_json_nested_objects);
	RUN_TEST_CASE(test_json_array);
	RUN_TEST_CASE(test_json_iterate_object);
}
