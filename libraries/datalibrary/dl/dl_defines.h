/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_DEFINES_H_INCLUDED
#define DL_DL_DEFINES_H_INCLUDED

#if defined(_MSC_VER)
	#define DL_FORCEINLINE __forceinline
	#define DL_DLL_EXPORT  __declspec(dllexport)
#elif defined(__GNUC__)
	#define DL_FORCEINLINE inline __attribute__((always_inline))
	#define DL_DLL_EXPORT
#else
	#error No supported compiler
#endif

// remove me!
#if defined(_MSC_VER)
	typedef unsigned __int32 dl_typeid_t;
#elif defined(__GNUC__)
	#include <stdint.h>
	typedef uint32_t dl_typeid_t;
#endif

#define DL_BITMASK(_Bits)                   ( (1ULL << (_Bits)) - 1ULL )
#define DL_BITRANGE(_MinBit,_MaxBit)		( ((1ULL << (_MaxBit)) | ((1ULL << (_MaxBit))-1ULL)) ^ ((1ULL << (_MinBit))-1ULL) )

#define DL_ZERO_BITS(_Target, _Start, _Bits)         ( (_Target) & ~DL_BITRANGE(_Start, (_Start) + (_Bits) - 1ULL) )
#define DL_EXTRACT_BITS(_Val, _Start, _Bits)         ( (_Val >> (_Start)) & DL_BITMASK(_Bits) )
#define DL_INSERT_BITS(_Target, _Val, _Start, _Bits) ( DL_ZERO_BITS(_Target, _Start, _Bits) | ( (DL_BITMASK(_Bits) & (_Val) ) << (_Start)) )

#endif // DL_DL_DEFINES_H_INCLUDED
