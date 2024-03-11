/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "cute_guid.h"
#include "cute_networking.h"

CF_Guid cf_make_guid()
{
	CF_Guid guid;
	cf_crypto_random_bytes(guid.data, sizeof(guid.data));
	return guid;
}
