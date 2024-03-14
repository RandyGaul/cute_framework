/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_RESULT_H
#define CF_RESULT_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define CF_RESULT_DEFS \
	CF_ENUM(RESULT_SUCCESS, 0) \
	CF_ENUM(RESULT_ERROR, -1) \

enum
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_RESULT_DEFS
	#undef CF_ENUM
};

/**
 * @struct   CF_Result
 * @category utility
 * @brief    Information about the result of a function, containing any potential error details.
 * @remarks  Check if a result is an error or not with `cf_is_error`.
 * @related  CF_Result cf_is_error cf_result_make cf_result_error cf_result_success
 */
typedef struct CF_Result
{
	/* @member Either 0 for success, or -1 for failure. */
	int code;

	/* @member String containing details about any error encountered. */
	const char* details;
} CF_Result;
// @end

/**
 * @function cf_is_error
 * @category utility
 * @brief    Returns true if a `CF_Result` contains an error.
 * @param    result       The result.
 * @related  CF_Result cf_is_error cf_result_make cf_result_error cf_result_success
 */
CF_INLINE bool cf_is_error(CF_Result result) { return result.code == CF_RESULT_ERROR; }

/**
 * @function cf_result_error
 * @category utility
 * @brief    Returns a `CF_Result` containing an error.
 * @param    details      Details about the error.
 * @related  CF_Result cf_is_error cf_result_make cf_result_error cf_result_success
 */
CF_INLINE CF_Result cf_result_error(const char* details) { CF_Result result; result.code = CF_RESULT_ERROR; result.details = details; return result; }

/**
 * @function cf_result_success
 * @category utility
 * @brief    Returns a `CF_Result` as a success, containing no error information.
 * @related  CF_Result cf_is_error cf_result_make cf_result_error cf_result_success
 */
CF_INLINE CF_Result cf_result_success() { CF_Result result; result.code = CF_RESULT_SUCCESS; result.details = NULL; return result; }

/**
 * @enum     CF_MessageBoxType
 * @category utility
 * @brief    Types of supported message boxes.
 * @related  CF_MessageBoxType cf_message_box_type_to_string cf_message_box
 */
#define CF_MESSAGE_BOX_TYPE_DEFS \
	/* @entry An error message box. */         \
	CF_ENUM(MESSAGE_BOX_TYPE_ERROR, 0)         \
	/* @entry A warning message box.. */       \
	CF_ENUM(MESSAGE_BOX_TYPE_WARNING, 1)       \
	/* @entry An informational message box. */ \
	CF_ENUM(MESSAGE_BOX_TYPE_INFORMATION, 2)   \
	/* @end */

typedef enum CF_MessageBoxType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_MESSAGE_BOX_TYPE_DEFS
	#undef CF_ENUM
} CF_MessageBoxType;

/**
 * @function cf_message_box_type_to_string
 * @category utility
 * @brief    Convert an enum `CF_MessageBoxType` to a c-style string.
 * @param    state        The state to convert to a string.
 * @related  CF_MessageBoxType cf_message_box_type_to_string cf_message_box
 */
CF_INLINE const char* cf_message_box_type_to_string(CF_MessageBoxType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_MESSAGE_BOX_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_message_box
 * @category utility
 * @brief    Displays a message in a new window.
 * @param    type       The type of the message box. See `CF_MessageBoxType`.
 * @param    title      Title string of the window.
 * @param    text       Text to display as the window's message.
 * @related  CF_MessageBoxType cf_message_box_type_to_string cf_message_box
 */
CF_API void CF_CALL cf_message_box(CF_MessageBoxType type, const char* title, const char* text);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Result = CF_Result;

enum : int
{
	#define CF_ENUM(K, V) K = V,
	CF_RESULT_DEFS
	#undef CF_ENUM
};

using MessageBoxType = CF_MessageBoxType;
#define CF_ENUM(K, V) CF_INLINE constexpr MessageBoxType K = CF_##K;
CF_MESSAGE_BOX_TYPE_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(MessageBoxType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_MESSAGE_BOX_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CF_INLINE bool is_error(Result error) { return cf_is_error(error); }

CF_INLINE Result result_failure(const char* details) { return cf_result_error(details); }
CF_INLINE Result result_success() { return cf_result_success(); }
CF_INLINE void message_box(MessageBoxType type, const char* title, const char* text) { return cf_message_box(type, title, text); }

}

#endif // CF_CPP

#endif // CF_RESULT_H
