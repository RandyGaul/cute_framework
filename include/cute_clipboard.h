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
CF_API char* CF_CALL cf_clipboard_get(void);

/**
 * @function cf_clipboard_set
 * @category input
 * @brief    Sets a UTF-8 string of the clipboard contents.
 * @related  cf_clipboard_get cf_clipboard_set
 */
CF_API CF_Result CF_CALL cf_clipboard_set(const char* string);

/**
 * @function cf_clipboard_set_data
 * @category input
 * @brief    Sets arbitrary data on the clipboard for the given MIME types.
 * @param    data            Pointer to the data to copy onto the clipboard.
 * @param    size            Size of the data in bytes.
 * @param    mime_types      Array of MIME type strings (e.g. "image/png").
 * @param    num_mime_types  Number of MIME type strings.
 * @return   Returns success or an error result.
 * @remarks  The data is copied internally. The same data is returned for any of the listed MIME types.
 * @related  cf_clipboard_set_data cf_clipboard_get_data cf_clipboard_has_data
 */
CF_API CF_Result CF_CALL cf_clipboard_set_data(const void* data, int size, const char** mime_types, int num_mime_types);

/**
 * @function cf_clipboard_get_data
 * @category input
 * @brief    Gets clipboard data for the given MIME type.
 * @param    mime_type  The MIME type to request (e.g. "image/png").
 * @param    size       Output parameter for the size of the returned data in bytes.
 * @return   Returns a malloc'd buffer with the clipboard data. Caller must free with `cf_free`. Returns NULL on failure.
 * @related  cf_clipboard_set_data cf_clipboard_get_data cf_clipboard_has_data
 */
CF_API void* CF_CALL cf_clipboard_get_data(const char* mime_type, int* size);

/**
 * @function cf_clipboard_has_data
 * @category input
 * @brief    Checks if the clipboard contains data for the given MIME type.
 * @param    mime_type  The MIME type to check (e.g. "image/png").
 * @return   Returns true if the clipboard has data for the MIME type.
 * @related  cf_clipboard_set_data cf_clipboard_get_data cf_clipboard_has_data
 */
CF_API bool CF_CALL cf_clipboard_has_data(const char* mime_type);

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
CF_INLINE CF_Result clipboard_set_data(const void* data, int size, const char** mime_types, int num_mime_types) { return cf_clipboard_set_data(data, size, mime_types, num_mime_types); }
CF_INLINE void* clipboard_get_data(const char* mime_type, int* size) { return cf_clipboard_get_data(mime_type, size); }
CF_INLINE bool clipboard_has_data(const char* mime_type) { return cf_clipboard_has_data(mime_type); }

}

#endif // CF_CPP

#endif // CF_CLIPBOARD_H
