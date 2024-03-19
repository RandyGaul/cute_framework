/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_VERSION_H
#define CF_VERSION_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#define CF_VERSION_STRING_COMPILED "Cute Framework Version 0.0"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CF_API const char* CF_CALL cf_version_string_linked();

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

CF_INLINE const char* version_string_linked() { return cf_version_string_linked(); }

}

#endif // CF_CPP

#endif // CF_VERSION_H
