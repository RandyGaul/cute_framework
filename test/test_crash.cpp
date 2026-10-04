/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute_c_runtime.h>
#include <cute/cute_sym.h>
#include <SDL3/SDL.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// The reporter end to end: the crashme sample crashes on purpose through the reporter's own
// `--cc-test` flag, a report lands in a directory of our choosing, and it resolves against the
// table cute-sym embedded in crashme to the function that performed the crash.

#ifdef CF_TEST_HAVE_CRASHME

static char s_crashme[1024];
static char s_dir[1024];

// SDL's base path, not cf_fs: the suites share one app, and tearing the file system down here
// would pull it out from under the tests that follow.
static bool s_paths(const char* kind)
{
	const char* base = SDL_GetBasePath();
#ifdef _WIN32
	snprintf(s_crashme, sizeof(s_crashme), "%scrashme.exe", base);
#else
	snprintf(s_crashme, sizeof(s_crashme), "%scrashme", base);
#endif
	snprintf(s_dir, sizeof(s_dir), "%scrash_test_%s", base, kind);
	return true;
}

static void s_rmdir_reports()
{
	int count = 0;
	char** files = SDL_GlobDirectory(s_dir, "*", 0, &count);
	for (int i = 0; files && i < count; ++i) {
		char path[2048];
		snprintf(path, sizeof(path), "%s/%s", s_dir, files[i]);
		SDL_RemovePath(path);
	}
	SDL_free(files);
	SDL_RemovePath(s_dir);
}

// Runs crashme to its end and returns the one report it left, read into memory, or NULL.
static char* s_run(const char* kind, char* report_path, size_t report_path_cap)
{
	s_rmdir_reports();
	SDL_CreateDirectory(s_dir);
	const char* args[] = { s_crashme, "--cc-test", kind, "--report-dir", s_dir, "--no-ask", NULL };
	SDL_Process* p = SDL_CreateProcess(args, false);
	if (!p) { printf("test_crash: cannot start %s: %s\n", s_crashme, SDL_GetError()); return NULL; }
	int code = 0;
	SDL_WaitProcess(p, true, &code);
	SDL_DestroyProcess(p);
	int count = 0;
	char** files = SDL_GlobDirectory(s_dir, "*.json", 0, &count);
	char* text = NULL;
	if (count == 1) {
		snprintf(report_path, report_path_cap, "%s/%s", s_dir, files[0]);
		size_t len = 0;
		text = (char*)SDL_LoadFile(report_path, &len);
	} else {
		printf("test_crash: %d reports for %s (expected 1)\n", count, kind);
	}
	SDL_free(files);
	return text;
}

static bool s_has(const char* text, const char* needle) { return strstr(text, needle) != NULL; }

TEST_CASE(test_crash_null_resolves)
{
	s_paths("null");
	char path[1100];
	char* text = s_run("null", path, sizeof(path));
	REQUIRE(text);
	REQUIRE(s_has(text, "\"kind\": \"crash\""));
	REQUIRE(s_has(text, "\"stack\""));
	REQUIRE(s_has(text, "\"offset\""));
	REQUIRE(s_has(text, "\"build_id\""));
	SDL_free(text);

	// Embedded in crashme by cf_symbols(crashme EMBED): the top frame names the crash site.
	REQUIRE(sym_resolve(path, NULL, NULL, NULL));
	size_t len = 0;
	text = (char*)SDL_LoadFile(path, &len);
	REQUIRE(text);
	REQUIRE(s_has(text, "cc_test_null_site"));
	REQUIRE(s_has(text, "cute_crash.h"));
	REQUIRE(s_has(text, "\"symbolic\""));
	SDL_free(text);
	s_rmdir_reports();
	return true;
}

TEST_CASE(test_crash_abort)
{
	s_paths("abort");
	char path[1100];
	char* text = s_run("abort", path, sizeof(path));
	REQUIRE(text);
	REQUIRE(s_has(text, "\"kind\": \"crash\""));
	REQUIRE(s_has(text, "\"fault\""));
	SDL_free(text);
	s_rmdir_reports();
	return true;
}

TEST_CASE(test_crash_thread)
{
	s_paths("thread");
	char path[1100];
	char* text = s_run("thread", path, sizeof(path));
	REQUIRE(text);
	REQUIRE(s_has(text, "\"kind\": \"crash\""));
	REQUIRE(s_has(text, "\"thread_name\": \"cc-test\""));
	SDL_free(text);
	s_rmdir_reports();
	return true;
}

TEST_CASE(test_crash_hang)
{
	s_paths("hang");
	char path[1100];
	char* text = s_run("hang", path, sizeof(path));
	REQUIRE(text);
	REQUIRE(s_has(text, "\"kind\": \"hang\""));
	REQUIRE(s_has(text, "\"stack\""));
	SDL_free(text);
	s_rmdir_reports();
	return true;
}

#endif // CF_TEST_HAVE_CRASHME

TEST_SUITE(test_crash)
{
#ifdef CF_TEST_HAVE_CRASHME
	RUN_TEST_CASE(test_crash_null_resolves);
	RUN_TEST_CASE(test_crash_abort);
	RUN_TEST_CASE(test_crash_thread);
	RUN_TEST_CASE(test_crash_hang);
#endif
}
