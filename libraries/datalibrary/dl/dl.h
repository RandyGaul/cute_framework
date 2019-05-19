/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_H_INCLUDED
#define DL_DL_H_INCLUDED

/*
	File: dl.h
*/

#include <stddef.h>
#include <dl/dl_defines.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*
	Handle: dl_ctx_t
*/
typedef struct dl_context* dl_ctx_t;

/*
	Enum: dl_error_t
		Error-codes from DL

	DL_ERROR_OK                                            - All went ok!
	DL_ERROR_MALFORMED_DATA                                - The data was not valid DL-data.
	DL_ERROR_VERSION_MISMATCH                              - Data created with another version of DL.
	DL_ERROR_OUT_OF_LIBRARY_MEMORY                         - Out of memory.
	DL_ERROR_OUT_OF_INSTANCE_MEMORY                        - Out of instance memory.
	DL_ERROR_DYNAMIC_SIZE_TYPES_AND_NO_INSTANCE_ALLOCATOR  - DL would need to do a dynamic allocation but has no allocator.
	DL_ERROR_TYPE_MISMATCH                                 - Expected type A but found type B.
	DL_ERROR_TYPE_NOT_FOUND                                - Could not find a requested type. Is the correct type library loaded?
	DL_ERROR_MEMBER_NOT_FOUND                              - Could not find a requested member of a type.
	DL_ERROR_BUFFER_TO_SMALL                               - Provided buffer is to small.
	DL_ERROR_ENDIAN_MISMATCH                               - Endianness of provided data is not the same as the platform's.
	DL_ERROR_BAD_ALIGNMENT                                 - One argument has a bad alignment that will break, for example, loaded data.
	DL_ERROR_UNSUPPORTED_OPERATION                         - The operation is not supported by dl-function.

	DL_ERROR_TXT_PARSE_ERROR                               - Syntax error while parsing txt-file. Check log for details.
	DL_ERROR_TXT_MEMBER_MISSING                            - A member is missing in a struct and in do not have a default value.
	DL_ERROR_TXT_MEMBER_SET_TWICE                          - A member is set twice in one struct.

	DL_ERROR_UTIL_FILE_NOT_FOUND                           - An argument-file is not found.
	DL_ERROR_UTIL_FILE_TYPE_MISMATCH                       - File type specified to read do not match file content.

	DL_ERROR_INTERNAL_ERROR                                - Internal error, contact dev!
*/
typedef enum
{
	DL_ERROR_OK,
	DL_ERROR_MALFORMED_DATA,
	DL_ERROR_VERSION_MISMATCH,
	DL_ERROR_OUT_OF_LIBRARY_MEMORY,
	DL_ERROR_OUT_OF_INSTANCE_MEMORY,
	DL_ERROR_DYNAMIC_SIZE_TYPES_AND_NO_INSTANCE_ALLOCATOR,
	DL_ERROR_OUT_OF_DEFAULT_VALUE_SLOTS,
	DL_ERROR_TYPE_MISMATCH,
	DL_ERROR_TYPE_NOT_FOUND,
	DL_ERROR_BUFFER_TO_SMALL,
	DL_ERROR_ENDIAN_MISMATCH,
	DL_ERROR_BAD_ALIGNMENT,
	DL_ERROR_INVALID_PARAMETER,
	DL_ERROR_INVALID_DEFAULT_VALUE,
	DL_ERROR_UNSUPPORTED_OPERATION,

	DL_ERROR_TXT_PARSE_ERROR,
	DL_ERROR_TXT_MISSING_MEMBER,
	DL_ERROR_TXT_MEMBER_SET_TWICE,
	DL_ERROR_TXT_INVALID_MEMBER,
	DL_ERROR_TXT_RANGE_ERROR,
	DL_ERROR_TXT_INVALID_MEMBER_TYPE,
	DL_ERROR_TXT_INVALID_ENUM_VALUE,
	DL_ERROR_TXT_MISSING_SECTION,
	DL_ERROR_TXT_MULTIPLE_MEMBERS_IN_UNION_SET,

	DL_ERROR_TYPELIB_MISSING_MEMBERS_IN_TYPE,

	DL_ERROR_UTIL_FILE_NOT_FOUND,
	DL_ERROR_UTIL_FILE_TYPE_MISMATCH,

	DL_ERROR_INTERNAL_ERROR
} dl_error_t;

/*
	Enum: dl_type_atom_t
		used in dl_type_t and determies the 'atom' type of a member.
*/
typedef enum
{
	DL_TYPE_ATOM_POD,
	DL_TYPE_ATOM_ARRAY,
	DL_TYPE_ATOM_INLINE_ARRAY,
	DL_TYPE_ATOM_BITFIELD,

	DL_TYPE_ATOM_CNT
} dl_type_atom_t;

/*
	Enum: dl_type_storage_t
		used in dl_type_t and determies the 'storage' type of a member.
*/
typedef enum
{
	DL_TYPE_STORAGE_INT8,
	DL_TYPE_STORAGE_INT16,
	DL_TYPE_STORAGE_INT32,
	DL_TYPE_STORAGE_INT64,
	DL_TYPE_STORAGE_UINT8,
	DL_TYPE_STORAGE_UINT16,
	DL_TYPE_STORAGE_UINT32,
	DL_TYPE_STORAGE_UINT64,
	DL_TYPE_STORAGE_FP32,
	DL_TYPE_STORAGE_FP64,
	DL_TYPE_STORAGE_ENUM_INT8,
	DL_TYPE_STORAGE_ENUM_INT16,
	DL_TYPE_STORAGE_ENUM_INT32,
	DL_TYPE_STORAGE_ENUM_INT64,
	DL_TYPE_STORAGE_ENUM_UINT8,
	DL_TYPE_STORAGE_ENUM_UINT16,
	DL_TYPE_STORAGE_ENUM_UINT32,
	DL_TYPE_STORAGE_ENUM_UINT64,
	DL_TYPE_STORAGE_STR,
	DL_TYPE_STORAGE_PTR,
	DL_TYPE_STORAGE_STRUCT,

	DL_TYPE_STORAGE_CNT
} dl_type_storage_t;

typedef enum
{
	DL_ENDIAN_BIG,
	DL_ENDIAN_LITTLE,
} dl_endian_t;

DL_FORCEINLINE dl_endian_t dl_endian_host()
{
	union { unsigned char c[4]; unsigned int  i; } test;
	test.i = 0xAABBCCDD;
	return test.c[0] == 0xAA ? DL_ENDIAN_BIG : DL_ENDIAN_LITTLE;
}

#define DL_ENDIAN_HOST dl_endian_host()

/*
	Function: dl_alloc_func
		Callback used by DL to allocate memory internally.

	Parameters:
		size      - number of bytes to allocate.
		alloc_ctx - same ptr that was passed to dl_context_create via dl_create_params.alloc_ctx.

	Return:
		Pointer to newly allocated memory.
*/
typedef void* (*dl_alloc_func)( size_t size, void* alloc_ctx );

/*
	Function: dl_realloc_func.
		Callback used by DL to reallocate a buffer to a different size.

	Parameters:
		ptr       - pointer to memory to reallocate, if 0x0 dl_realloc_func should act as dl_alloc_func.
		size      - number of bytes to allocate.
		old_size  - previous size of allocation.
		alloc_ctx - same ptr that was passed to dl_context_create via dl_create_params.alloc_ctx.

	Return:
		Pointer to newly allocated/reallocated memory.
*/
typedef void* (*dl_realloc_func)( void* ptr, size_t size, size_t old_size, void* alloc_ctx );

/*
	Function: dl_free_func
		Callback used by DL to free memory allocated by either dl_alloc_func or dl_realloc_func.

	Parameters:
		ptr       - pointer to memory to free, if 0x0 this should be a no-op.
		alloc_ctx - same ptr that was passed to dl_context_create via dl_create_params.alloc_ctx.
*/
typedef void  (*dl_free_func) ( void* ptr, void* alloc_ctx );

/*
	Function: dl_error_msg_handler
		Callback used by DL to report error-messages.

	Parameters:
		msg     - error message that DL log.
		msg_ctx - same ptr that was passed to dl_context_create via dl_create_params.error_msg_ctx.
*/
typedef void  (*dl_error_msg_handler)( const char* msg, void* msg_ctx );

/*
	Struct: dl_create_params_t
		Passed with initialization parameters to dl_context_create.
		This struct is open to change in later versions of dl.

	Members:
		alloc_func   - function called by dl to allocate memory, set to 0x0 to use malloc
		realloc_func - function called by dl to reallocate a memory buffer, set to 0x0 to use realloc.
		free_func    - function called by dl to free memory, set to 0x0 to use free
		alloc_ctx    - parameter passed to alloc_func/free_func for userdata.

		error_msg_func - callback used to report errors in more detail than error-codes
		                 to the user, set to 0x0 to ignore error-strings.
		error_msg_ctx  - data passed to error_msg_func as user-data.

	Note:
		As a user you might replace the internal memory allocation function by using alloc_func, realloc_func
		and free_func.
		If you set alloc_func you are required to set free_func as well and can optionally set realloc_func.
		If no realloc_func is set but alloc_func and free_func is set DL will fallback on alloc_func + memcpy.
*/
typedef struct dl_create_params
{
	dl_alloc_func   alloc_func;
	dl_realloc_func realloc_func;
	dl_free_func    free_func;
	void*           alloc_ctx;

	dl_error_msg_handler error_msg_func;
	void*                error_msg_ctx;
} dl_create_params_t;

/*
	Macro: DL_CREATE_PARAMS_SET_DEFAULT
		The preferred way to initialize dl_create_params_t is with this
		This macro will set default values that might not be optimal but
		is supposed to support all usecases of dl.
		This should be used to not get uninitialized members if create_params_t
		is extended.

	Example:
		dl_create_params_t p;
		DL_CREATE_PARAMS_SET_DEFAULT(p);
		p.alloc_func = my_func
*/
#define DL_CREATE_PARAMS_SET_DEFAULT( params ) \
		params.alloc_func   = 0x0; \
		params.realloc_func = 0x0; \
		params.free_func    = 0x0; \
		params.alloc_ctx    = 0x0; \
		params.error_msg_func = 0x0; \
		params.error_msg_ctx  = 0x0;

/*
	Group: Context
*/

/*
	Function: dl_context_create
		Creates a context.

	Parameters:
		dl_ctx        - Ptr to instance to create.
		create_params - Parameters to control the construction of the dl context. See DL_CREATE_PARAMS_SET_DEFAULT
		                for usage.
*/
dl_error_t DL_DLL_EXPORT dl_context_create( dl_ctx_t* dl_ctx, dl_create_params_t* create_params );

/*
	Function: dl_context_destroy
		Destroys a context and free all memory allocated with the DLAllocFuncs-functions.
*/
dl_error_t DL_DLL_EXPORT dl_context_destroy( dl_ctx_t dl_ctx );

/*
	Function: dl_context_load_type_library
		Load a type-library from bin-data into the context for use.
		One context can have multiple type libraries loaded and reference types within the different ones.

	Parameters:
		dl_ctx        - Context to load type-library into.
		lib_data      - Pointer to binary-data with type-library.
		lib_data_size - Size of lib_data.
*/
dl_error_t DL_DLL_EXPORT dl_context_load_type_library( dl_ctx_t dl_ctx, const unsigned char* lib_data, size_t lib_data_size );


/*
	Group: Load
*/

/*
	Function: dl_instance_load
		Loads an instance from packed format to usable by system.

	Parameters:
		dl_ctx               - DL-context to use when loading instance.
		dl_typeid            - Type of instance in the packed data.
		instance             - Ptr where to load the instance to.
		instance_size        - Size of buffer pointed to by instance.
		packed_instance      - Ptr to binary data to load from.
		packed_instance_size - Size of the buffer pointed to by packed_instance
		consumed             - Number of bytes consumed to load an instance is returned here, 0x0 to ignore.

	Note:
		Packed instance to load is required to be in current platform endian, if not DL_ERROR_ENDIAN_ERROR will be returned.
*/
dl_error_t DL_DLL_EXPORT dl_instance_load( dl_ctx_t             dl_ctx,          dl_typeid_t type,
                                           void*                instance,        size_t instance_size,
                                           const unsigned char* packed_instance, size_t packed_instance_size,
                                           size_t*              consumed );

/*
	Function: dl_instance_load_inplace
		Loads an instance inplace from packed data. This mean that the instance will be loaded in the same
		memory area where the packed instance is stored.
		Packed memory-area will need to be valid memory until loaded instance is no longer used.

	Parameters:
		dl_ctx               - DL-context to use when loading instance.
		dl_typeid            - Type of instance in the packed data.
		packed_instance      - Packed instance-data to load.
		packed_instance_size - Size of buffer pointed to by packed_instance.
		loaded_instance      - Loaded instance will be returned here.
		consumed             - Number of bytes consumed to load an instance is returned here, 0x0 to ignore.

	Note:
		Some small memory-waste will be incurred by this function since some header-data will be left in memory.
*/
dl_error_t DL_DLL_EXPORT dl_instance_load_inplace( dl_ctx_t       dl_ctx,          dl_typeid_t type,
												   unsigned char* packed_instance, size_t      packed_instance_size,
												   void**         loaded_instance, size_t*     consumed );

/*
	Group: Store
*/
/*
	Function: dl_instace_calc_size
		Calculate size needed to store instance.

	Parameters:
		dl_ctx   - Context to load type-library into.
		type     - Type id for type to store.
		instance - Ptr to instance to calculate size of.
		out_size - Ptr where to store the amount of bytes needed to store the instances.
*/
dl_error_t DL_DLL_EXPORT dl_instance_calc_size( dl_ctx_t dl_ctx, dl_typeid_t type, void* instance, size_t* out_size);

/*
	Function: dl_instace_store
		Store the instances.

	Parameters:
		dl_ctx          - Context to load type-library into.
		type            - Type id for type to store.
		instance        - Ptr to instance to store.
		out_buffer      - Ptr to memory-area where to store the instances.
		out_buffer_size - Size of out_buffer.
		produced_bytes  - number of bytes that would have been written to out_buffer if it was large enough.

	Return:
		DL_ERROR_OK on success. Storing an instance to a 0-sized out_buffer is thought of as a success as
		it can be used to calculate the size of a stored instance.
		If out_buffer_size is > 0 but smaller than the required size to store the instance, an error is
		returned.

	Note:
		The instance after pack will be in current platform endian.

		Function can be used to calculate the amount of bytes that will be produced if storing an instance
		by setting out_buffer_size to 0.
*/
dl_error_t DL_DLL_EXPORT dl_instance_store( dl_ctx_t       dl_ctx,     dl_typeid_t type,            const void* instance,
											unsigned char* out_buffer, size_t      out_buffer_size, size_t*     produced_bytes );


/*
	Group: Util
*/

/*
	Function: dl_error_to_string
		Converts EDLError to string.
*/
DL_DLL_EXPORT const char* dl_error_to_string( dl_error_t error );

typedef struct dl_instance_info
{
	unsigned int load_size;
	unsigned int ptrsize;
	dl_endian_t  endian;
	dl_typeid_t  root_type;
} dl_instance_info_t;

/*
	Function: dl_instance_get_info
		Fetch information about a packed dl-instance.

	Parameters:
		packed_instance      - Ptr to memory-area where packed instance is to be found.
		packed_instance_size - Size of packed_instance.
		out_info             - Ptr to dl_instance_info where to return info.

	Return:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_instance_get_info( const unsigned char* packed_instance, size_t packed_instance_size, dl_instance_info_t* out_info );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DL_DL_H_INCLUDED
