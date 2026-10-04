/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute_c_runtime.h>
#include <SDL3/SDL.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define CUTE_SYM_IMPLEMENTATION
#include <cute/cute_sym.h>

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#elif defined(__APPLE__)
#	include <mach-o/dyld.h>
#else
#	include <link.h>
#endif

// A CUTESYM v1 image written by hand, straight from the format (records 16/24/32 bytes, naturally
// aligned): the reader and the lookup are checked against the layout, not against the builder that
// also writes it.
struct SymImage
{
	unsigned char* bytes;
	size_t len;
};

static void s_put32(unsigned char* p, uint32_t v) { for (int i = 0; i < 4; ++i) p[i] = (unsigned char)(v >> (8 * i)); }
static void s_put64(unsigned char* p, uint64_t v) { for (int i = 0; i < 8; ++i) p[i] = (unsigned char)(v >> (8 * i)); }
static size_t s_round8(size_t n) { return (n + 7) & ~(size_t)7; }

static SymImage s_make_image()
{
	// strings: "" main a.c helper inner
	const char* strings = "\0main\0a.c\0helper\0inner\0";
	const uint32_t strings_len = 1 + 5 + 4 + 7 + 6;
	const uint32_t s_main = 1, s_ac = 6, s_helper = 10, s_inner = 17;
	const uint32_t func_count = 2, line_count = 4, inline_count = 2, file_count = 1;
	size_t off_funcs = 96;
	size_t off_lines = s_round8(off_funcs + func_count * 16);
	size_t off_inlines = s_round8(off_lines + line_count * 24);
	size_t off_files = s_round8(off_inlines + inline_count * 32);
	size_t off_strings = s_round8(off_files + file_count * 4);
	size_t total = s_round8(off_strings + strings_len);
	unsigned char* b = (unsigned char*)calloc(total, 1);
	memcpy(b, "CUTESYM\0", 8);
	s_put32(b + 8, 1);
	s_put32(b + 12, 1);
	for (int i = 0; i < 20; ++i) b[16 + i] = (unsigned char)(0xa0 + i);
	b[36] = 20;
	b[37] = 3;
	s_put32(b + 40, func_count);
	s_put32(b + 44, line_count);
	s_put32(b + 48, inline_count);
	s_put32(b + 52, file_count);
	s_put32(b + 56, strings_len);
	s_put32(b + 60, (uint32_t)off_funcs);
	s_put32(b + 64, (uint32_t)off_lines);
	s_put32(b + 68, (uint32_t)off_inlines);
	s_put32(b + 72, (uint32_t)off_files);
	s_put32(b + 76, (uint32_t)off_strings);
	s_put32(b + 80, (uint32_t)total);
	unsigned char* f = b + off_funcs;
	s_put64(f, 0x1000); s_put32(f + 8, 0x100); s_put32(f + 12, s_main);
	s_put64(f + 16, 0x2000); s_put32(f + 24, 0x40); s_put32(f + 28, s_helper);
	unsigned char* l = b + off_lines;
	uint64_t starts[4] = { 0x1000, 0x1010, 0x1040, 0x2000 };
	uint32_t lens[4] = { 0x10, 0x30, 0xc0, 0x40 };
	uint32_t lines[4] = { 10, 11, 20, 50 };
	for (int i = 0; i < 4; ++i) { s_put64(l + i * 24, starts[i]); s_put32(l + i * 24 + 8, lens[i]); s_put32(l + i * 24 + 12, s_ac); s_put32(l + i * 24 + 16, lines[i]); }
	unsigned char* in = b + off_inlines;
	// helper inlined into main over [0x1010, 0x1040) called from a.c:11; inner inlined into that over [0x1020, 0x1030) from a.c:30.
	s_put64(in, 0x1010); s_put32(in + 8, 0x30); s_put32(in + 12, s_helper); s_put32(in + 16, s_ac); s_put32(in + 20, 11); s_put32(in + 24, 0xFFFFFFFFu);
	s_put64(in + 32, 0x1020); s_put32(in + 40, 0x10); s_put32(in + 44, s_inner); s_put32(in + 48, s_ac); s_put32(in + 52, 30); s_put32(in + 56, 0);
	s_put32(b + off_files, s_ac);
	memcpy(b + off_strings, strings, strings_len);
	SymImage img = { b, total };
	return img;
}

TEST_CASE(test_sym_format_lookup)
{
	SymImage img = s_make_image();
	sym_table t;
	REQUIRE(sym_read(img.bytes, img.len, &t));
	REQUIRE(t.func_count == 2);
	REQUIRE(t.line_count == 4);
	REQUIRE(t.inline_count == 2);
	REQUIRE(t.build_id_len == 20 && t.build_id[0] == 0xa0 && t.build_id[19] == 0xb3);

	sym_frame fr;
	sym_frame inl[4];
	int n = 0;
	REQUIRE(sym_lookup(&t, 0x1004, &fr, inl, 4, &n));
	REQUIRE(!strcmp(fr.function, "main"));
	REQUIRE(!strcmp(fr.file, "a.c"));
	REQUIRE(fr.line == 10);
	REQUIRE(n == 0);

	// Inside both inline ranges: innermost first.
	REQUIRE(sym_lookup(&t, 0x1024, &fr, inl, 4, &n));
	REQUIRE(fr.line == 11);
	REQUIRE(n == 2);
	REQUIRE(!strcmp(inl[0].function, "inner") && inl[0].line == 30);
	REQUIRE(!strcmp(inl[1].function, "helper") && inl[1].line == 11);

	REQUIRE(sym_lookup(&t, 0x2010, &fr, inl, 4, &n));
	REQUIRE(!strcmp(fr.function, "helper") && fr.line == 50 && n == 0);

	// Between functions: nothing.
	REQUIRE(!sym_lookup(&t, 0x1800, &fr, inl, 4, &n));

	// A truncated image is rejected, not read past.
	REQUIRE(!sym_read(img.bytes, img.len - 1, &t));
	REQUIRE(!sym_read(img.bytes, 40, &t));
	free(img.bytes);
	return true;
}

#ifdef CF_TEST_HAVE_SYMBOLS // Only with the tool on and debug info in this build; the format test above is the whole suite otherwise.
// The function the self test looks up: a call inside it captures its own return address, which
// is an address inside the body (a function pointer may be a linker thunk under incremental
// linking), and the line it carries is the call's.
#if defined(_MSC_VER)
#	include <intrin.h>
#	define TEST_NOINLINE __declspec(noinline)
#	define TEST_RETURN_ADDRESS() _ReturnAddress()
#else
#	define TEST_NOINLINE __attribute__((noinline))
#	define TEST_RETURN_ADDRESS() __builtin_return_address(0)
#endif
static const void* s_known_return;
TEST_NOINLINE static void s_capture_return(void) { s_known_return = TEST_RETURN_ADDRESS(); }
TEST_NOINLINE int test_sym_known_function(void)
{
	s_capture_return();
	volatile int line = __LINE__;
	return line;
}

static uint64_t s_module_relative(const void* addr)
{
#ifdef _WIN32
	return (uint64_t)((const char*)addr - (const char*)GetModuleHandleW(NULL));
#elif defined(__APPLE__)
	return (uint64_t)((uintptr_t)addr - (uintptr_t)_dyld_get_image_vmaddr_slide(0));
#else
	struct Bias { uintptr_t bias; bool set; } b = { 0, false };
	dl_iterate_phdr([](struct dl_phdr_info* info, size_t, void* data) {
		struct Bias* bb = (struct Bias*)data;
		if (!bb->set) { bb->bias = info->dlpi_addr; bb->set = true; }
		return 1;
	}, &b);
	return (uint64_t)((uintptr_t)addr - b.bias);
#endif
}

static void s_log(void* udata, const char* msg) { (void)udata; printf("cute_sym: %s\n", msg); }

// The tests binary's own debug info, read by the tool's library: names and lines come back for a
// function we can point at.
TEST_CASE(test_sym_self)
{
	const char* base = SDL_GetBasePath(); // Not cf_fs: the suites share one app and its file system.
	char path[1024];
#ifdef _WIN32
	snprintf(path, sizeof(path), "%s%s", base, "tests.exe");
#else
	snprintf(path, sizeof(path), "%s%s", base, "tests");
#endif
	sym_table t;
	bool built = sym_build(path, NULL, &t, s_log, NULL);
	REQUIRE(built);
	REQUIRE(t.func_count > 100);
	REQUIRE(t.line_count > 100);

	int expected_line = test_sym_known_function();
	uint64_t addr = s_module_relative(s_known_return) - 1; // The resolver's rule for a return address.
	sym_frame fr;
	int n = 0;
	REQUIRE(sym_lookup(&t, addr, &fr, NULL, 0, &n));
	REQUIRE(strstr(fr.function, "test_sym_known_function") != NULL);
	REQUIRE(strstr(fr.file, "test_sym.cpp") != NULL);
	REQUIRE((int)fr.line >= expected_line - 3 && (int)fr.line <= expected_line + 1);

	// Written, read back, same answer.
	char out[1100];
	snprintf(out, sizeof(out), "%s.test.sym", path);
	REQUIRE(sym_write(&t, out));
	sym_table u;
	REQUIRE(sym_read_file(out, &u));
	REQUIRE(u.build_id_len == t.build_id_len && !memcmp(u.build_id, t.build_id, t.build_id_len));
	sym_frame fr2;
	REQUIRE(sym_lookup(&u, addr, &fr2, NULL, 0, &n));
	REQUIRE(!strcmp(fr.function, fr2.function) && fr.line == fr2.line);
	sym_free(&u);
	sym_free(&t);
	remove(out);
	return true;
}

#endif // CF_TEST_HAVE_SYMBOLS

TEST_SUITE(test_sym)
{
	RUN_TEST_CASE(test_sym_format_lookup);
#ifdef CF_TEST_HAVE_SYMBOLS
	RUN_TEST_CASE(test_sym_self);
#endif
}
