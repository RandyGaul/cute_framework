/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

static bool s_basic_hit = false;
static const char* s_basic_effect_name = NULL;
static double s_basic_number = 0;
static const char* s_basic_string = NULL;
static CF_Color s_basic_color;

TEST_CASE(test_markups_basic)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	s_basic_hit = false;
	auto markup_info_fn = [](const char* text, CF_MarkupInfo info, const CF_TextEffect* fx) {
		CF_UNUSED(text);
		s_basic_hit = true;
		s_basic_effect_name = info.effect_name;
		s_basic_number = cf_text_effect_get_number(fx, "number", 0);
		s_basic_string = cf_text_effect_get_string(fx, "string", NULL);
		s_basic_color = cf_text_effect_get_color(fx, "color", cf_color_white());
	};
	const char* text = "<fake number=123 string=\"maybe a string\" color=#ff00ff12>does this work?</fake>";
	cf_text_get_markup_info(markup_info_fn, text, V2(0,0), -1);

	REQUIRE(s_basic_hit);
	REQUIRE(s_basic_effect_name == sintern("fake"));
	REQUIRE(s_basic_number == 123.0);
	REQUIRE(!CF_STRCMP(s_basic_string, "maybe a string"));
	REQUIRE(s_basic_color == cf_make_color_rgba(0xff, 0x00, 0xff, 0x12));

	destroy_app();

	return true;
}

static bool hit = false;

TEST_CASE(test_markups_bad_inputs)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	hit = false;
	auto markup_info_fn = [](const char* text, CF_MarkupInfo info, const CF_TextEffect* fx) {
		CF_UNUSED(text);
		CF_UNUSED(info);
		CF_UNUSED(fx);
		hit = true;
	};
	text_get_markup_info(markup_info_fn, "<broken>test string/broken>", V2(0,0));
	text_get_markup_info(markup_info_fn, "<broken>test stringbroken>", V2(0,0));
	text_get_markup_info(markup_info_fn, "<broken>test stringbroken", V2(0,0));
	text_get_markup_info(markup_info_fn, "<brokentest stringbroken", V2(0,0));
	text_get_markup_info(markup_info_fn, "brokentest stringbroken", V2(0,0));
	text_get_markup_info(markup_info_fn, "<broken>test string<broken>", V2(0,0));
	text_get_markup_info(markup_info_fn, "<broken>test string<broken", V2(0,0));
	text_get_markup_info(markup_info_fn, "<brokentest string</broken>", V2(0,0));
	text_get_markup_info(markup_info_fn, "broken>test string</broken>", V2(0,0));
	text_get_markup_info(markup_info_fn, "<<>>", V2(0,0));
	text_get_markup_info(markup_info_fn, "te<broken>st/broken> string", V2(0,0));
	text_get_markup_info(markup_info_fn, "te<broken>st/broken> st<>>><rin<>g", V2(0,0));
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

TEST_CASE(test_markups_font_size_vertical_metrics)
{
	// A larger <font size=N> span must grow the measured vertical extent and the
	// line-to-line advance, so measurement boxes and wrapped lines stay trustworthy.
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	push_font("Calibri");
	push_font_size(32);

	// Single line: a big size span is taller than the base-size line.
	float base_h = text_height("Hello", -1);
	float big_h = text_height("<font size=64>Hello</font>", -1);
	REQUIRE(big_h > base_h);

	// A big span embedded mid-line still lifts the whole line's height.
	float mixed_h = text_height("a<font size=64>B</font>c", -1);
	REQUIRE(mixed_h > base_h);

	// text_size().y agrees with text_height().
	CF_V2 big_sz = text_size("<font size=64>Hello</font>", -1);
	REQUIRE(big_sz.y == big_h);

	// Multi-line: a big second line makes the block taller than an all-base block,
	// i.e. the line advance follows the tallest style on each line (no collision).
	float two_base = text_height("small\nsmall", -1);
	float two_big = text_height("small\n<font size=64>BIG</font>", -1);
	REQUIRE(two_big > two_base);

	// A pure-base string is unaffected by the feature.
	REQUIRE(text_height("small\nsmall", -1) == two_base);

	pop_font_size();
	pop_font();
	destroy_app();
	return true;
}

TEST_CASE(test_markups_empty_tag_no_crash)
{
	// A degenerate tag with no name (e.g. "<>") makes the parser produce an empty tag
	// name; interning its NULL c_str() used to crash. Measuring must be safe instead.
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	push_font("Calibri");
	push_font_size(24);

	// None of these should crash; each returns a finite, non-negative width.
	REQUIRE(text_width("<>", -1) >= 0);
	REQUIRE(text_width("a<>b", -1) >= 0);
	REQUIRE(text_width("<=x>text</>", -1) >= 0);
	REQUIRE(text_width("< >spaced", -1) >= 0);

	pop_font_size();
	pop_font();
	destroy_app();
	return true;
}

TEST_SUITE(test_markups)
{
	RUN_TEST_CASE(test_markups_basic);
	RUN_TEST_CASE(test_markups_bad_inputs);
	RUN_TEST_CASE(test_markups_font_style);
	RUN_TEST_CASE(test_markups_font_size_vertical_metrics);
	RUN_TEST_CASE(test_markups_empty_tag_no_crash);
}
