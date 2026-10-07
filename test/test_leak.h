/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// CRT heap accounting for Debug MSVC builds. Elsewhere test_leak_check_enabled() is false and
// leak-count tests skip.
//
//     TestLeakCheck leak;
//     test_leak_begin(&leak);
//     ... make + destroy things ...
//     REQUIRE(test_leak_report(&leak, "thing make/destroy") == 0);
//
// main() tags SDL's allocations as client blocks, so the normal-block counts below are CF's
// own heap and SDL's leaks (SDL 3.4.0's D3D12 backend leaks a bytecode copy per compute
// pipeline) are reported separately.

#ifndef TEST_LEAK_H
#define TEST_LEAK_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>

struct TestLeakCheck { _CrtMemState start; };

inline bool test_leak_check_enabled() { return true; }
inline void test_leak_begin(TestLeakCheck* check) { _CrtMemCheckpoint(&check->start); }

// Net growth of one block type since test_leak_begin.
inline int64_t test_leak_delta(TestLeakCheck* check, bool bytes, int block_type)
{
	_CrtMemState now;
	_CrtMemCheckpoint(&now);
	const size_t* a = bytes ? now.lSizes : now.lCounts;
	const size_t* b = bytes ? check->start.lSizes : check->start.lCounts;
	return (int64_t)(a[block_type] - b[block_type]);
}

inline int64_t test_leak_bytes(TestLeakCheck* check) { return test_leak_delta(check, true, _NORMAL_BLOCK); }
inline int64_t test_leak_blocks(TestLeakCheck* check) { return test_leak_delta(check, false, _NORMAL_BLOCK); }
inline int64_t test_leak_sdl_bytes(TestLeakCheck* check) { return test_leak_delta(check, true, _CLIENT_BLOCK); }

inline void test_leak_dump(TestLeakCheck* check) { _CrtMemDumpAllObjectsSince(&check->start); }

inline bool test_leak_dump_requested()
{
	const char* dump = getenv("CF_TEST_LEAK_DUMP");
	return dump && *dump == '1';
}

// Returns the net bytes leaked, printing a summary when nonzero (and every surviving block
// with CF_TEST_LEAK_DUMP=1).
inline int64_t test_leak_report(TestLeakCheck* check, const char* what)
{
	int64_t bytes = test_leak_bytes(check);
	if (bytes) {
		fprintf(stderr, "%s leaked %lld bytes in %lld blocks\n", what, (long long)bytes, (long long)test_leak_blocks(check));
		if (test_leak_dump_requested()) test_leak_dump(check);
	}
	return bytes;
}
#else
struct TestLeakCheck { int unused; };

inline bool test_leak_check_enabled() { return false; }
inline void test_leak_begin(TestLeakCheck*) { }
inline int64_t test_leak_bytes(TestLeakCheck*) { return 0; }
inline int64_t test_leak_blocks(TestLeakCheck*) { return 0; }
inline int64_t test_leak_sdl_bytes(TestLeakCheck*) { return 0; }
inline void test_leak_dump(TestLeakCheck*) { }
inline bool test_leak_dump_requested() { return false; }
inline int64_t test_leak_report(TestLeakCheck*, const char*) { return 0; }
#endif

#endif // TEST_LEAK_H
