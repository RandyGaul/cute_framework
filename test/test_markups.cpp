/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

TEST_CASE(test_markups_basic)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	auto markup_info_fn = [](const char* text, CF_MarkupInfo info, const CF_TextEffect* fx) -> bool {
		REQUIRE(info.effect_name == sintern("fake"));
		double number = cf_text_effect_get_number(fx, "number", 0);
		const char* string = cf_text_effect_get_string(fx, "string", NULL);
		CF_Color color = cf_text_effect_get_color(fx, "color", cf_color_white());
		REQUIRE(number == 123.0);
		REQUIRE(!CF_STRCMP(string, "maybe a string"));
		REQUIRE(color == cf_make_color_rgba(0xff, 0x00, 0xff, 0x12));
		return true;
	};
	const char* text = "<fake number=123 string=\"maybe a string\" color=#ff00ff12>does this work?</fake>";
	cf_text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, text, V2(0,0), -1);

	destroy_app();

	return true;
}

static bool hit = false;

TEST_CASE(test_markups_bad_inputs)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	hit = false;
	auto markup_info_fn = [](const char* text, CF_MarkupInfo info, const CF_TextEffect* fx) {
		CF_UNUSED(text);
		CF_UNUSED(info);
		CF_UNUSED(fx);
		hit = true;
		return true;
	};
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "<broken>test string/broken>", V2(0,0));
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "<broken>test stringbroken>", V2(0,0));
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "<broken>test stringbroken", V2(0,0));
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "<brokentest stringbroken", V2(0,0));
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "brokentest stringbroken", V2(0,0));
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "<broken>test string<broken>", V2(0,0));
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "<broken>test string<broken", V2(0,0));
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "<brokentest string</broken>", V2(0,0));
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "broken>test string</broken>", V2(0,0));
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "<<>>", V2(0,0));
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "te<broken>st/broken> string", V2(0,0));
	text_get_markup_info((cf_text_markup_info_fn*)&markup_info_fn, "te<broken>st/broken> st<>>><rin<>g", V2(0,0));
	REQUIRE(hit == false);

	destroy_app();

	return true;
}

// Embedded ProggyClean for style-tag measurement without depending on VFS mounts.
#include "../samples/proggy.h"

TEST_CASE(test_markups_font_style)
{
	// Bold/italic style tags swap to a different loaded face and use that face's metrics.
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	// Load a second face distinct from the default Calibri.
	REQUIRE(!is_error(make_font_from_memory(proggy_data, proggy_sz, "ProggyClean")));

	text_effect_set_font("b", "ProggyClean");

	push_font("Calibri");
	push_font_size(32);

	float plain_w = text_width("Hello", -1);
	float mapped_w = text_width("<b>Hello</b>", -1);
	float font_tag_w = text_width("<font name=\"ProggyClean\">Hello</font>", -1);
	float size_tag_w = text_width("<font name=\"ProggyClean\" size=64>Hello</font>", -1);

	// Mapped <b> and explicit <font name> should match each other and differ from base face.
	REQUIRE(mapped_w == font_tag_w);
	REQUIRE(mapped_w != plain_w);
	// Larger size override should measure wider than the default size span.
	REQUIRE(size_tag_w > font_tag_w);

	// Nested: inner set_font override wins over outer <font>.
	float nested_w = text_width("<font name=\"Calibri\"><b>Hello</b></font>", -1);
	REQUIRE(nested_w == mapped_w);

	pop_font_size();
	pop_font();
	destroy_app();
	return true;
}

TEST_SUITE(test_markups)
{
	// Don't submit this to GitHub as the build machines can't init a graphics context.
#if 0
	RUN_TEST_CASE(test_markups_basic);
	RUN_TEST_CASE(test_markups_bad_inputs);
	RUN_TEST_CASE(test_markups_font_style);
#endif
}
