/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_SERIALIZE_INTERNAL_H
#define CF_SERIALIZE_INTERNAL_H

#include <cute_defines.h>
#include <cute_networking.h>

CF_INLINE void cf_write_uint8(uint8_t** p, uint8_t value)
{
	**p = value;
	++(*p);
}

CF_INLINE void cf_write_uint16(uint8_t** p, uint16_t value)
{
	(*p)[0] = value & 0xFF;
	(*p)[1] = value >> 8;
	*p += 2;
}

CF_INLINE void cf_write_uint32(uint8_t** p, uint32_t value)
{
	(*p)[0] = value & 0xFF;
	(*p)[1] = (value >> 8 ) & 0xFF;
	(*p)[2] = (value >> 16) & 0xFF;
	(*p)[3] = value >> 24;
	*p += 4;
}

CF_INLINE void cf_write_float(uint8_t** p, float value)
{
	union
	{
		uint32_t as_uint32;
		float as_float;
	} val;
	val.as_float = value;
	cf_write_uint32(p, val.as_uint32);
}

CF_INLINE void cf_write_uint64(uint8_t** p, uint64_t value)
{
	(*p)[0] = value & 0xFF;
	(*p)[1] = (value >> 8 ) & 0xFF;
	(*p)[2] = (value >> 16) & 0xFF;
	(*p)[3] = (value >> 24) & 0xFF;
	(*p)[4] = (value >> 32) & 0xFF;
	(*p)[5] = (value >> 40) & 0xFF;
	(*p)[6] = (value >> 48) & 0xFF;
	(*p)[7] = value >> 56;
	*p += 8;
}

CF_INLINE void cf_write_bytes(uint8_t** p, const uint8_t* byte_array, int num_bytes)
{
	for (int i = 0; i < num_bytes; ++i)
	{
		cf_write_uint8(p, byte_array[i]);
	}
}

CF_INLINE void cf_write_address(uint8_t** p, CF_Address endpoint)
{
	cf_write_uint8(p, (uint8_t)endpoint.type);
	if (endpoint.type == (cn_address_type_t)CF_ADDRESS_TYPE_IPV4) {
		cf_write_uint8(p, endpoint.u.ipv4[0]);
		cf_write_uint8(p, endpoint.u.ipv4[1]);
		cf_write_uint8(p, endpoint.u.ipv4[2]);
		cf_write_uint8(p, endpoint.u.ipv4[3]);
	} else if (endpoint.type == (cn_address_type_t)CF_ADDRESS_TYPE_IPV6) {
		cf_write_uint16(p, endpoint.u.ipv6[0]);
		cf_write_uint16(p, endpoint.u.ipv6[1]);
		cf_write_uint16(p, endpoint.u.ipv6[2]);
		cf_write_uint16(p, endpoint.u.ipv6[3]);
		cf_write_uint16(p, endpoint.u.ipv6[4]);
		cf_write_uint16(p, endpoint.u.ipv6[5]);
		cf_write_uint16(p, endpoint.u.ipv6[6]);
		cf_write_uint16(p, endpoint.u.ipv6[7]);
	} else {
		CF_ASSERT(0);
	}
	cf_write_uint16(p, endpoint.port);
}

CF_INLINE void cf_write_key(uint8_t** p, const CF_CryptoKey* key)
{
	cf_write_bytes(p, (const uint8_t*)key, sizeof(*key));
}

CF_INLINE void cf_write_fourcc(uint8_t** p, const char* fourcc)
{
	cf_write_uint8(p, fourcc[0]);
	cf_write_uint8(p, fourcc[1]);
	cf_write_uint8(p, fourcc[2]);
	cf_write_uint8(p, fourcc[3]);
}

CF_INLINE uint8_t cf_read_uint8(uint8_t** p)
{
	uint8_t value = **p;
	++(*p);
	return value;
}

CF_INLINE uint16_t cf_read_uint16(uint8_t** p)
{
	uint16_t value;
	value = (*p)[0];
	value |= (((uint16_t)((*p)[1])) << 8);
	*p += 2;
	return value;
}

CF_INLINE uint32_t cf_read_uint32(uint8_t** p)
{
	uint32_t value;
	value  = (*p)[0];
	value |= (((uint32_t)((*p)[1])) << 8);
	value |= (((uint32_t)((*p)[2])) << 16);
	value |= (((uint32_t)((*p)[3])) << 24);
	*p += 4;
	return value;
}

CF_INLINE float cf_read_float(uint8_t** p)
{
	union
	{
		uint32_t as_uint32;
		float as_float;
	} val;
	val.as_uint32 = cf_read_uint32(p);
	return val.as_float;
}

CF_INLINE uint64_t cf_read_uint64(uint8_t** p)
{
	uint64_t value;
	value  = (*p)[0];
	value |= (((uint64_t)((*p)[1])) << 8 );
	value |= (((uint64_t)((*p)[2])) << 16);
	value |= (((uint64_t)((*p)[3])) << 24);
	value |= (((uint64_t)((*p)[4])) << 32);
	value |= (((uint64_t)((*p)[5])) << 40);
	value |= (((uint64_t)((*p)[6])) << 48);
	value |= (((uint64_t)((*p)[7])) << 56);
	*p += 8;
	return value;
}

CF_INLINE void cf_read_bytes(uint8_t** p, uint8_t* byte_array, int num_bytes)
{
	for (int i = 0; i < num_bytes; ++i)
	{
		byte_array[i] = cf_read_uint8(p);
	}
}

CF_INLINE CF_Address cf_read_address(uint8_t** p)
{
	CF_Address endpoint;
	endpoint.type = (CF_AddressType)cf_read_uint8(p);
	if (endpoint.type == CF_ADDRESS_TYPE_IPV4) {
		endpoint.u.ipv4[0] = cf_read_uint8(p);
		endpoint.u.ipv4[1] = cf_read_uint8(p);
		endpoint.u.ipv4[2] = cf_read_uint8(p);
		endpoint.u.ipv4[3] = cf_read_uint8(p);
	} else if (endpoint.type == CF_ADDRESS_TYPE_IPV6) {
		endpoint.u.ipv6[0] = cf_read_uint16(p);
		endpoint.u.ipv6[1] = cf_read_uint16(p);
		endpoint.u.ipv6[2] = cf_read_uint16(p);
		endpoint.u.ipv6[3] = cf_read_uint16(p);
		endpoint.u.ipv6[4] = cf_read_uint16(p);
		endpoint.u.ipv6[5] = cf_read_uint16(p);
		endpoint.u.ipv6[6] = cf_read_uint16(p);
		endpoint.u.ipv6[7] = cf_read_uint16(p);
	} else {
		CF_ASSERT(0);
	}
	endpoint.port = cf_read_uint16(p);
	return endpoint;
}

CF_INLINE CF_CryptoKey cf_read_key(uint8_t** p)
{
	CF_CryptoKey key;
	cf_read_bytes(p, (uint8_t*)&key, sizeof(key));
	return key;
}

CF_INLINE void cf_read_fourcc(uint8_t** p, uint8_t* fourcc)
{
	fourcc[0] = cf_read_uint8(p);
	fourcc[1] = cf_read_uint8(p);
	fourcc[2] = cf_read_uint8(p);
	fourcc[3] = cf_read_uint8(p);
}

#ifdef CF_CPP

namespace Cute
{

}

#endif // CF_CPP

#endif // CF_SERIALIZE_INTERNAL_H
