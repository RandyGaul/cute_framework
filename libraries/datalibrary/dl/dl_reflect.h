/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_REFLECT_H_INCLUDED
#define DL_DL_REFLECT_H_INCLUDED

/*
	File: dl_reflect.h
		Functions used to get information about types and type-members. Mostly designed for
		use when binding DL towards other languages.
*/

#include <dl/dl.h>

#include <stdint.h>

/*
	Struct: dl_type_context_info_t
		Struct used to retrieve information about the dl_context
*/
typedef struct dl_type_context_info
{
	unsigned int num_types;
	unsigned int num_enums;
} dl_type_context_info_t;

/*
	Struct: dl_type_info_t
		Struct used to retrieve information about a specific DL-type.
*/
typedef struct dl_type_info
{
	dl_typeid_t  tid;
	const char*  name;
	const char*  comment;
	unsigned int size;
	unsigned int alignment;
	unsigned int member_count;
	unsigned int is_extern : 1;
	unsigned int is_union : 1;
	unsigned int should_verify : 1;
} dl_type_info_t;

/*
	Struct: dl_member_info_t
		Struct used to retrieve information about a specific DL-member.
*/
typedef struct dl_member_info
{
	const char*       name;
	const char*       comment;
	dl_type_atom_t    atom;
	dl_type_storage_t storage;
	dl_typeid_t       type_id;
	unsigned int      size;
	unsigned int      alignment;
	unsigned int      offset;
	unsigned int      array_count;
	unsigned int      bits;
	unsigned int 	  is_const : 1;
	unsigned int 	  should_verify : 1;
} dl_member_info_t;

/*
	Struct: dl_type_info_t
		Struct used to retrieve information about a specific DL-enum.
*/
typedef struct dl_enum_info
{
	dl_typeid_t       tid;
	const char*       name;
	const char*       comment;
	dl_type_storage_t storage;
	unsigned int      value_count;
	unsigned int      is_extern : 1;
} dl_enum_info_t;

/*
	Struct: dl_member_info_t
		Struct used to retrieve information about a specific DL-enum-value.
*/
typedef struct dl_enum_value_info
{
	const char*  name;
	union
	{
		int8_t   i8;
		int16_t  i16;
		int32_t  i32;
		int64_t  i64;
		uint8_t  u8;
		uint16_t u16;
		uint32_t u32;
		uint64_t u64;
	} value;
} dl_enum_value_info_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
	Function: dl_reflect_context_info
		Get the info about the context and the typelibraries loaded into it.

	Parameters:
		dl_ctx - The dl-context to get info from.
		info   - Struct to fill.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_context_info( dl_ctx_t dl_ctx, dl_type_context_info_t* info );

/*
	Function: dl_reflect_loaded_typeids
		Get the typeid:s of all loaded types in the context.

	Parameters:
		dl_ctx         - The dl-context to fetch loaded types from.
		out_types      - Buffer to return loaded types in.
		out_types_size - Size of out_types.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_loaded_typeids( dl_ctx_t dl_ctx, dl_typeid_t* out_types, unsigned int out_types_size );

/*
	Function: dl_reflect_loaded_types
		Get the dl_type_info_t:s of all loaded types in the context.

	Parameters:
		dl_ctx         - The dl-context to fetch loaded types from.
		out_types      - Buffer to return loaded types in.
		out_types_size - Size of out_types.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_loaded_types( dl_ctx_t dl_ctx, dl_type_info_t* out_types, unsigned int out_types_size );

/*
	Function: dl_reflect_loaded_enums
		Get the typeid:s of all loaded enums in the context.

	Parameters:
		dl_ctx         - The dl-context to fetch loaded enums from.
		out_enums      - Buffer to return loaded enums in.
		out_enums_size - Size of out_enums.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_loaded_enumids( dl_ctx_t dl_ctx, dl_typeid_t* out_enums, unsigned int out_enums_size );

/*
	Function: dl_reflect_loaded_enums
		Get the dl_enum_info_t:s of all loaded enums in the context.

	Parameters:
		dl_ctx         - The dl-context to fetch loaded enums from.
		out_enums      - Buffer to return loaded enums in.
		out_enums_size - Size of out_enums.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_loaded_enums( dl_ctx_t dl_ctx, dl_enum_info_t* out_enums, unsigned int out_enums_size );

/*
	Function: dl_reflect_get_type_id
		Find typeid of a specified type by name.

	Parameters:
		dl_ctx      - A valid handle to a DLContext to do the type-lookup in.
		type_ame    - Name of type to lookup.
		out_type_id - TypeID returned here.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_get_type_id( dl_ctx_t dl_ctx, const char* type_name, dl_typeid_t* out_type );

/*
	Function: dl_reflect_get_type_info
		Retrieve information about a certain type in a type-library.

	Parameters:
		dl_ctx        - A valid handle to a DLContext
		type          - TypeID of the type to get information about.
		out_type_info - Ptr to struct to fill with type-information.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_get_type_info( dl_ctx_t dl_ctx, dl_typeid_t type, dl_type_info_t* out_type_info );

/*
	Function: dl_reflect_get_enum_info
		Retrieve information about a certain enum in a type-library.

	Parameters:
		dl_ctx        - A valid handle to a DLContext
		type          - TypeID of the enum to get information about.
		out_enum_info - Ptr to struct to fill with value-information.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t DL_DLL_EXPORT dl_reflect_get_enum_info( dl_ctx_t dl_ctx, dl_typeid_t type, dl_enum_info_t* out_enum_info );

/*
	Function: dl_reflect_get_type_members
		Retrieve information about members of a type in a type-library.

	Parameters:
		dl_ctx        - A valid handle to a DLContext
		type          - TypeID of the type to get information about.
		out_members   - Ptr to array to fill with information about the members of the type.
		members_size  - Size of _pMembers.

	Returns:
		DL_ERROR_OK on success, DL_ERROR_BUFFER_TO_SMALL if out_members do not fit all members, or other error if appropriate!
*/
dl_error_t DL_DLL_EXPORT dl_reflect_get_type_members( dl_ctx_t dl_ctx, dl_typeid_t type, dl_member_info_t* out_members, unsigned int out_members_size );

/*
	Function: dl_reflect_get_enum_values
		Retrieve values of a certain enum.

	Parameters:
		dl_ctx          - A valid handle to a DLContext
		type            - TypeID of the enum to get values of.
		out_values      - Ptr to array to fill with enum values.
		out_values_size - Size of out_values.

	Returns:
		DL_ERROR_OK on success, DL_ERROR_BUFFER_TO_SMALL if out_members do not fit all members, or other error if appropriate!
*/
dl_error_t DL_DLL_EXPORT dl_reflect_get_enum_values( dl_ctx_t dl_ctx, dl_typeid_t type, dl_enum_value_info_t* out_values, unsigned int out_values_size );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DL_DL_REFLECT_H_INCLUDED
