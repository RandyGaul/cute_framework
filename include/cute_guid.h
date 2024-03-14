/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_GUID_H
#define CF_GUID_H

#include "cute_defines.h"
#include "cute_c_runtime.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Guid
 * @category utility
 * @brief    A general purpose unique identifier.
 * @related  CF_Guid cf_make_guid cf_guid_equal
 */
typedef struct CF_Guid
{
	/* @member The raw bytes of the Guid. */
	uint8_t data[16];
} CF_Guid;
// @end

/**
 * @function cf_make_guid
 * @category utility
 * @brief    Returns a new `CF_Guid`.
 * @remarks  The bytes are generated in a cryptographically secure way.
 * @related  CF_Guid cf_make_guid cf_guid_equal
 */
CF_API CF_Guid CF_CALL cf_make_guid();

/**
 * @function cf_guid_equal
 * @category utility
 * @brief    Returns true if two `CF_Guid`'s are equal, false otherwise.
 * @param    a         A guid to compare.
 * @param    b         A guid to compare.
 * @related  CF_Guid cf_make_guid cf_guid_equal
 */
CF_INLINE bool cf_guid_equal(CF_Guid a, CF_Guid b) { return !CF_MEMCMP(&a, &b, sizeof(a)); }

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Guid = CF_Guid;
CF_INLINE bool operator==(Guid a, Guid b) { return cf_guid_equal(a, b); }
CF_INLINE bool operator!=(Guid a, Guid b) { return !cf_guid_equal(a, b); }

CF_INLINE Guid make_guid() { return cf_make_guid(); }

}

#endif // CF_CPP

#endif // CF_GUID_H
