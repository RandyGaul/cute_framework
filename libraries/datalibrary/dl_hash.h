#ifndef DL_HASH_H_INCLUDED
#define DL_HASH_H_INCLUDED

#include <dl/dl_defines.h>

// TODO: replace with murmur3

DL_FORCEINLINE static uint32_t dl_internal_hash_buffer( const uint8_t* buffer, size_t bytes )
{
	uint32_t hash = 5381;
	for (unsigned int i = 0; i < bytes; i++)
		hash = (hash * uint32_t(33)) + *((uint8_t*)buffer + i);
	return hash - 5381;
}

DL_FORCEINLINE static uint32_t dl_internal_hash_string( const char* str )
{
	uint32_t hash = 5381;
	for (unsigned int i = 0; str[i] != 0; i++)
		hash = (hash * uint32_t(33)) + uint32_t(str[i]);
	return hash - 5381; // So empty string == 0
}


#endif // DL_HASH_H_INCLUDED
