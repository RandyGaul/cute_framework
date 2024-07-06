/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

/* Run all the path related C string APIs. */
TEST_CASE(test_path_c)
{
	const char* s = spfname("../root/file.txt");
	REQUIRE(sequ(s, "file.txt"));
	sfree(s);

	s = spfname("./file.txt");
	REQUIRE(sequ(s, "file.txt"));
	sfree(s);

	s = spfname("file.txt");
	REQUIRE(sequ(s, "file.txt"));
	sfree(s);

	s = spfname("file");
	REQUIRE(sequ(s, "file"));
	sfree(s);

	s = spfname("/.txt");
	REQUIRE(sequ(s, ".txt"));
	sfree(s);

	s = spfname("/root/");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = spfname("/.root/");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = spfname_no_ext("../root/file.txt");
	REQUIRE(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("../file.txt");
	REQUIRE(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("./file.txt");
	REQUIRE(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("/file.txt");
	REQUIRE(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("file.txt");
	REQUIRE(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("/file");
	REQUIRE(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("/root/");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = spfname_no_ext("/.root/");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = spfname_no_ext("/.txt");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = spext("../root/file.txt");
	REQUIRE(sequ(s, ".txt"));
	sfree(s);

	s = spext("../root/file");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = sppop("../root/file.txt");
	REQUIRE(sequ(s, "../root"));
	sfree(s);

	s = sppop("../root");
	REQUIRE(sequ(s, ".."));
	sfree(s);

	s = sppop("..");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = sppop("/folder/file.txt");
	REQUIRE(sequ(s, "/folder"));
	sfree(s);

	s = sppop("/folder");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = spcompact("/real_long_file_name.txt", 15);
	REQUIRE(sequ(s, "/real_long_f..."));
	sfree(s);

	s = spcompact("/folder/to/file.txt", 10);
	REQUIRE(sequ(s, ".../fol..."));
	sfree(s);

	s = spcompact("/folder/to/file.txt", 17);
	REQUIRE(sequ(s, "/fold.../file.txt"));
	sfree(s);

	s = spdir_of("/example/a/b/c/file.txt");
	REQUIRE(sequ(s, "/c"));
	sfree(s);

	s = spdir_of("/example/file.txt");
	REQUIRE(sequ(s, "/example"));
	sfree(s);

	s = spdir_of("/file.txt");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = spdir_of("../file.txt");
	REQUIRE(sequ(s, ".."));
	sfree(s);

	s = spdir_of("./file.txt");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = spdir_of("..");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = spdir_of("../");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = spdir_of("./");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = spdir_of(".");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = spdir_of("/");
	REQUIRE(sequ(s, NULL));
	sfree(s);

	s = sptop_of("/example/a/b/c/file.txt");
	REQUIRE(sequ(s, "/example"));
	sfree(s);

	s = sptop_of("/example/file.txt");
	REQUIRE(sequ(s, "/example"));
	sfree(s);

	s = sptop_of("/file.txt");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = sptop_of("/a");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = sptop_of("/");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = sptop_of("./");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = sptop_of("../");
	REQUIRE(sequ(s, "/"));
	sfree(s);

	s = spnorm("/first.last/.hidden");
	REQUIRE(sequ(s, "/first.last/.hidden"));
	sfree(s);

	return true;
}

TEST_SUITE(test_path)
{
	RUN_TEST_CASE(test_path_c);
}
