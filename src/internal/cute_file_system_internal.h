/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_FILE_SYSTEM_INTERNAL_H
#define CF_FILE_SYSTEM_INTERNAL_H

#include <cute_result.h>

CF_API CF_Result CF_CALL cf_fs_init(const char* argv0);
CF_API void CF_CALL cf_fs_destroy();

#ifdef CF_CPP

namespace Cute
{

}

#endif // CF_CPP

#endif // CF_FILE_SYSTEM_INTERNAL_H
