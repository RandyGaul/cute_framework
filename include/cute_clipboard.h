/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_CLIPBOARD_H
#define CF_CLIPBOARD_H

#include "cute_defines.h"
#include "cute_result.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @function cf_clipboard_get
 * @category input
 * @brief    Returns a UTF-8 string of the clipboard contents.
 * @related  cf_clipboard_get cf_clipboard_set
 */
CF_API char* CF_CALL cf_clipboard_get();

/**
 * @function cf_clipboard_set
 * @category input
 * @brief    Sets a UTF-8 string of the clipboard contents.
 * @related  cf_clipboard_get cf_clipboard_set
 */
CF_API CF_Result CF_CALL cf_clipboard_set(const char* string);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

CF_INLINE char* clipboard_get() { return cf_clipboard_get(); }
CF_INLINE CF_Result clipboard_set(const char* string) { return cf_clipboard_set(string); }

}

#endif // CF_CPP

#endif // CF_CLIPBOARD_H
