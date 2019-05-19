/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_SWAP_H_INCLUDED
#define DL_DL_SWAP_H_INCLUDED

static inline int8_t  dl_swap_endian_int8 ( int8_t  val ) { return val; }
static inline int16_t dl_swap_endian_int16( int16_t val ) { return (int16_t)( ( ( val & 0x00FF ) << 8 )  | ( ( val & 0xFF00 ) >> 8 ) ); }
static inline int32_t dl_swap_endian_int32( int32_t val ) { return ( ( val & 0x00FF ) << 24 ) | ( ( val & 0xFF00 ) << 8) | ( ( val >> 8 ) & 0xFF00 ) | ( ( val >> 24 ) & 0x00FF ); }
static inline int64_t dl_swap_endian_int64( int64_t val )
{
	union { int32_t m_i32[2]; int64_t m_i64; } conv;
	conv.m_i64 = val;
	int32_t tmp   = dl_swap_endian_int32( conv.m_i32[0] );
	conv.m_i32[0] = dl_swap_endian_int32( conv.m_i32[1] );
	conv.m_i32[1] = tmp;
	return conv.m_i64;
}
static inline uint8_t  dl_swap_endian_uint8 ( uint8_t  val ) { return val; }
static inline uint16_t dl_swap_endian_uint16( uint16_t val ) { return (uint16_t)( ( ( val & 0x00FF ) << 8 )  | ( val & 0xFF00 ) >> 8 ); }
static inline uint32_t dl_swap_endian_uint32( uint32_t val ) { return ( ( val & 0x00FF ) << 24 ) | ( ( val & 0xFF00 ) << 8) | ( ( val >> 8 ) & 0xFF00 ) | ( ( val >> 24 ) & 0x00FF ); }
static inline uint64_t dl_swap_endian_uint64( uint64_t val )
{
	union { uint32_t m_u32[2]; uint64_t m_u64; } conv;
	conv.m_u64 = val;
	uint32_t tmp  = dl_swap_endian_uint32( conv.m_u32[0] );
	conv.m_u32[0] = dl_swap_endian_uint32( conv.m_u32[1] );
	conv.m_u32[1] = tmp;
	return conv.m_u64;
}

static inline float dl_swap_endian_fp32( float f )
{
	union { uint32_t m_u32; float m_fp32; } conv;
	conv.m_fp32 = f;
	conv.m_u32  = dl_swap_endian_uint32( conv.m_u32 );
	return conv.m_fp32;
}

static inline double dl_swap_endian_fp64( double f )
{
	union { uint64_t m_u64; double m_fp64; } conv;
	conv.m_fp64 = f;
	conv.m_u64  = dl_swap_endian_uint64( conv.m_u64 );
	return conv.m_fp64;
}

#endif // DL_DL_SWAP_H_INCLUDED
