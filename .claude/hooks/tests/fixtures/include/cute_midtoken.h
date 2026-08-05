/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_MIDTOKEN_H
#define CF_MIDTOKEN_H

/**
 * @function cf_midtoken_noop
 * @category test
 * @brief    Fixture with @ symbols that are not doc tags (mid-token).
 * @return   Always returns 0.
 */
// see http://example.com/page@fragment and array@index
int cf_midtoken_noop(void);

#endif // CF_MIDTOKEN_H
