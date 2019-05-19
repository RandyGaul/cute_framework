/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_CONVERT_H_INCLUDED
#define DL_DL_CONVERT_H_INCLUDED

/*
	File: dl_convert.h
		Exposes functionality to convert packed dl-instances between different formats
		regarding pointer-size and endianness.
*/

#include <dl/dl.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*
	Function: dl_convert
		Converts a packed instance to an other format.

	Parameters:
		dl_ctx               - Handle to valid DL-context.
		type                 - DL-type expected to be found in packed instance.
		packed_instance      - Ptr to memory-area where packed instance is to be found.
		packed_instance_size - Size of packed_instance.
		out_instance         - Ptr to memory-area where to place the converted instance.
		out_instance_size    - Size of out_instance_size.
		out_endian           - Endian to convert the packed instance to.
		out_ptr_size         - Size in bytes of pointers after conversions, valid values 4 and 8.
*/
dl_error_t DL_DLL_EXPORT dl_convert( dl_ctx_t       dl_ctx,          dl_typeid_t type,
                                     unsigned char* packed_instance, size_t      packed_instance_size,
                                     unsigned char* out_instance,    size_t      out_instance_size,
                                     dl_endian_t    out_endian,      size_t      out_ptr_size,
                                     size_t*        produced_bytes );

/*
	Function: dl_convert_inplace
		Converts a packed instance to an other format inplace.

	Parameters:
		dl_ctx               - Handle to valid DL-context.
		type                 - DL-type expected to be found in packed instance.
		packed_instance      - Ptr to memory-area where packed instance is to be found.
		packed_instance_size - Size of packed_instance.
		out_endian           - Endian to convert the packed instance to.
		out_ptr_size         - Size in bytes of pointers after conversions, valid values 4 and 8.
		produced_bytes       - Ptr where new size of instance will be returned. Can be set to 0x0.

	Note:
		Function is restricted to converting endianness and converting 8-byte ptr:s to 4-byte ptr:s

	Return:
		DL_ERROR_OK on success. DL_ERROR_UNSUPORTED_OPERATION is returned if trying to convert to format not
		supported, for example converting 4-byte ptrs to 8-byte.
*/
dl_error_t DL_DLL_EXPORT dl_convert_inplace( dl_ctx_t dl_ctx,                dl_typeid_t type,
                                             unsigned char* packed_instance, size_t      packed_instance_size,
                                             dl_endian_t    out_endian,      size_t      out_ptr_size,
                                             size_t*        produced_bytes );

/*
	Function: dl_convert_calc_size
		Calculates size of an instance after _PtrSize-conversion.

	Parameters:
		dl_ctx               - Handle to valid DL-context.
		type                 - DL-type expected to be found in packed instance.
		packed_instance      - Ptr to memory-area where packed instance is to be found.
		packed_instance_size - Size of _pData.
		out_ptr_size         - Size in bytes of pointers after conversions, valid values 4 and 8.
		out_size             - Ptr where to store the calculated size.
*/
dl_error_t DL_DLL_EXPORT dl_convert_calc_size( dl_ctx_t       dl_ctx,          dl_typeid_t type,
                                               unsigned char* packed_instance, size_t      packed_instance_size,
                                               size_t         out_ptr_size,    size_t*     out_size );

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // DL_DL_CONVERT_H_INCLUDED
