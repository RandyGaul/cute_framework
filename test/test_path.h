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

#include <cute.h>
using namespace Cute;

CF_TEST_CASE(test_path, "Run all the path related C string APIs.");
int test_path()
{
	const char* s = spfname("../root/file.txt");
	CF_TEST_ASSERT(sequ(s, "file.txt"));
	sfree(s);

	s = spfname("./file.txt");
	CF_TEST_ASSERT(sequ(s, "file.txt"));
	sfree(s);

	s = spfname("file.txt");
	CF_TEST_ASSERT(sequ(s, NULL));
	sfree(s);

	s = spfname(".txt");
	CF_TEST_ASSERT(sequ(s, NULL));
	sfree(s);

	s = spfname_no_ext("../root/file.txt");
	CF_TEST_ASSERT(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("../file.txt");
	CF_TEST_ASSERT(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("./file.txt");
	CF_TEST_ASSERT(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("/file.txt");
	CF_TEST_ASSERT(sequ(s, "file"));
	sfree(s);

	s = spfname_no_ext("file.txt");
	CF_TEST_ASSERT(sequ(s, NULL));
	sfree(s);

	s = spfname_no_ext("/file");
	CF_TEST_ASSERT(sequ(s, NULL));
	sfree(s);

	s = spfname_no_ext("/.txt");
	CF_TEST_ASSERT(sequ(s, NULL));
	sfree(s);

	s = spext("../root/file.txt");
	CF_TEST_ASSERT(sequ(s, ".txt"));
	sfree(s);

	s = spext("../root/file");
	CF_TEST_ASSERT(sequ(s, NULL));
	sfree(s);

	s = sppop("../root/file.txt");
	CF_TEST_ASSERT(sequ(s, "../root"));
	sfree(s);

	s = sppop("../root");
	CF_TEST_ASSERT(sequ(s, ".."));
	sfree(s);

	s = sppop("..");
	CF_TEST_ASSERT(sequ(s, "/"));
	sfree(s);

	s = sppop("/folder/file.txt");
	CF_TEST_ASSERT(sequ(s, "/folder"));
	sfree(s);

	s = sppop("/folder");
	CF_TEST_ASSERT(sequ(s, "/"));
	sfree(s);

	s = spcompact("/real_long_file_name.txt", 15);
	CF_TEST_ASSERT(sequ(s, "/real_long_f..."));
	sfree(s);

	s = spcompact("/folder/to/file.txt", 10);
	CF_TEST_ASSERT(sequ(s, ".../fol..."));
	sfree(s);

	s = spcompact("/folder/to/file.txt", 17);
	CF_TEST_ASSERT(sequ(s, "/fold.../file.txt"));
	sfree(s);

	s = spdir_of("/example/a/b/c/file.txt");
	CF_TEST_ASSERT(sequ(s, "/c"));
	sfree(s);

	s = spdir_of("/example/file.txt");
	CF_TEST_ASSERT(sequ(s, "/example"));
	sfree(s);

	s = spdir_of("/file.txt");
	CF_TEST_ASSERT(sequ(s, "/"));
	sfree(s);

	s = spdir_of("../file.txt");
	CF_TEST_ASSERT(sequ(s, ".."));
	sfree(s);

	s = spdir_of("./file.txt");
	CF_TEST_ASSERT(sequ(s, "/"));
	sfree(s);

	s = spdir_of("..");
	CF_TEST_ASSERT(sequ(s, NULL));
	sfree(s);

	s = spdir_of("../");
	CF_TEST_ASSERT(sequ(s, NULL));
	sfree(s);

	s = spdir_of("./");
	CF_TEST_ASSERT(sequ(s, NULL));
	sfree(s);

	s = spdir_of(".");
	CF_TEST_ASSERT(sequ(s, NULL));
	sfree(s);

	s = spdir_of("/");
	CF_TEST_ASSERT(sequ(s, NULL));
	sfree(s);

	s = sptop_of("/example/a/b/c/file.txt");
	CF_TEST_ASSERT(sequ(s, "/example"));
	sfree(s);

	s = sptop_of("/example/file.txt");
	CF_TEST_ASSERT(sequ(s, "/example"));
	sfree(s);

	s = sptop_of("/file.txt");
	CF_TEST_ASSERT(sequ(s, "/"));
	sfree(s);

	s = sptop_of("/a");
	CF_TEST_ASSERT(sequ(s, "/"));
	sfree(s);

	s = sptop_of("/");
	CF_TEST_ASSERT(sequ(s, "/"));
	sfree(s);

	s = sptop_of("./");
	CF_TEST_ASSERT(sequ(s, "/"));
	sfree(s);

	s = sptop_of("../");
	CF_TEST_ASSERT(sequ(s, "/"));
	sfree(s);

	return 0;
}
