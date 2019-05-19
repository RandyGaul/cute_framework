#include <dl/dl_typelib.h>
#include "dl_internal_util.h"
#include "dl_types.h"

static dl_error_t dl_internal_load_type_library_defaults( dl_ctx_t       dl_ctx,
														  const uint8_t* default_data,
														  unsigned int   default_data_size )
{
	if( default_data_size == 0 ) return DL_ERROR_OK;

	if( dl_ctx->default_data != 0x0 )
		return DL_ERROR_OUT_OF_DEFAULT_VALUE_SLOTS;

	dl_ctx->default_data = (uint8_t*)dl_alloc( &dl_ctx->alloc, default_data_size );
	dl_ctx->default_data_size = default_data_size;

	memcpy( dl_ctx->default_data, default_data, default_data_size );

	return DL_ERROR_OK;
}

static void dl_internal_read_typelibrary_header( dl_typelib_header* header, const uint8_t* data )
{
	memcpy(header, data, sizeof(dl_typelib_header));

	if(DL_ENDIAN_HOST == DL_ENDIAN_BIG)
	{
		header->id           = dl_swap_endian_uint32( header->id );
		header->version      = dl_swap_endian_uint32( header->version );

		header->type_count       = dl_swap_endian_uint32( header->type_count );
		header->enum_count       = dl_swap_endian_uint32( header->enum_count );
		header->member_count     = dl_swap_endian_uint32( header->member_count );
		header->enum_value_count = dl_swap_endian_uint32( header->enum_value_count );
		header->enum_alias_count = dl_swap_endian_uint32( header->enum_alias_count );

		header->default_value_size   = dl_swap_endian_uint32( header->default_value_size );
	}
}

static void dl_endian_swap_type_desc( dl_type_desc* desc )
{
	desc->size[DL_PTR_SIZE_32BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_32BIT] );
	desc->size[DL_PTR_SIZE_64BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_64BIT] );
	desc->alignment[DL_PTR_SIZE_32BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_32BIT] );
	desc->alignment[DL_PTR_SIZE_64BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_64BIT] );
	desc->member_count                 = dl_swap_endian_uint32( desc->member_count );
}

static void dl_endian_swap_enum_desc( dl_enum_desc* desc )
{
	desc->value_count = dl_swap_endian_uint32( desc->value_count );
	desc->value_start = dl_swap_endian_uint32( desc->value_start );
}

static void dl_endian_swap_member_desc( dl_member_desc* desc )
{
	desc->type                         = (dl_type_t)dl_swap_endian_uint32( (uint32_t)desc->type );
	desc->type_id	                   = dl_swap_endian_uint32( desc->type_id );
	desc->size[DL_PTR_SIZE_32BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_32BIT] );
	desc->size[DL_PTR_SIZE_64BIT]      = dl_swap_endian_uint32( desc->size[DL_PTR_SIZE_64BIT] );
	desc->offset[DL_PTR_SIZE_32BIT]    = dl_swap_endian_uint32( desc->offset[DL_PTR_SIZE_32BIT] );
	desc->offset[DL_PTR_SIZE_64BIT]    = dl_swap_endian_uint32( desc->offset[DL_PTR_SIZE_64BIT] );
	desc->alignment[DL_PTR_SIZE_32BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_32BIT] );
	desc->alignment[DL_PTR_SIZE_64BIT] = dl_swap_endian_uint32( desc->alignment[DL_PTR_SIZE_64BIT] );
	desc->default_value_offset         = dl_swap_endian_uint32( desc->default_value_offset );
}

static void dl_endian_swap_enum_value_desc( dl_enum_value_desc* desc )
{
	desc->value = dl_swap_endian_uint64( desc->value );
}

template <typename T>
static inline T* dl_realloc_array(dl_allocator* alloc, T* ptr, size_t new_size, size_t old_size)
{
	return (T*)dl_realloc(alloc, ptr, new_size * sizeof(T), old_size * sizeof(T));
}

dl_error_t dl_context_load_type_library( dl_ctx_t dl_ctx, const unsigned char* lib_data, size_t lib_data_size )
{
	if(lib_data_size < sizeof(dl_typelib_header))
		return DL_ERROR_MALFORMED_DATA;

	dl_typelib_header header;
	dl_internal_read_typelibrary_header(&header, lib_data);

	if( header.id      != DL_TYPELIB_ID )      return DL_ERROR_MALFORMED_DATA;
	if( header.version != DL_TYPELIB_VERSION ) return DL_ERROR_VERSION_MISMATCH;

	size_t types_lookup_offset     = sizeof(dl_typelib_header);
	size_t enums_lookup_offset     = types_lookup_offset + sizeof( dl_typeid_t ) * header.type_count;
	size_t types_offset            = enums_lookup_offset + sizeof( dl_typeid_t ) * header.enum_count;
	size_t enums_offset            = types_offset        + sizeof( dl_type_desc ) * header.type_count;
	size_t members_offset          = enums_offset        + sizeof( dl_enum_desc ) * header.enum_count;
	size_t enum_values_offset      = members_offset      + sizeof( dl_member_desc ) * header.member_count;
	size_t enum_aliases_offset     = enum_values_offset  + sizeof( dl_enum_value_desc ) * header.enum_value_count;
	size_t defaults_offset         = enum_aliases_offset + sizeof( dl_enum_alias_desc ) * header.enum_alias_count;
	size_t typedata_strings_offset = defaults_offset + header.default_value_size;

	dl_ctx->type_ids         = dl_realloc_array( &dl_ctx->alloc, dl_ctx->type_ids,         dl_ctx->type_count + header.type_count,                       dl_ctx->type_count );
	dl_ctx->type_descs       = dl_realloc_array( &dl_ctx->alloc, dl_ctx->type_descs,       dl_ctx->type_count + header.type_count,                       dl_ctx->type_count );
	dl_ctx->enum_ids         = dl_realloc_array( &dl_ctx->alloc, dl_ctx->enum_ids,         dl_ctx->enum_count + header.enum_count,                       dl_ctx->enum_count );
	dl_ctx->enum_descs       = dl_realloc_array( &dl_ctx->alloc, dl_ctx->enum_descs,       dl_ctx->enum_count + header.enum_count,                       dl_ctx->enum_count );
	dl_ctx->member_descs     = dl_realloc_array( &dl_ctx->alloc, dl_ctx->member_descs,     dl_ctx->member_count + header.member_count,                   dl_ctx->member_count );
	dl_ctx->enum_value_descs = dl_realloc_array( &dl_ctx->alloc, dl_ctx->enum_value_descs, dl_ctx->enum_value_count + header.enum_value_count,           dl_ctx->enum_value_count );
	dl_ctx->enum_alias_descs = dl_realloc_array( &dl_ctx->alloc, dl_ctx->enum_alias_descs, dl_ctx->enum_alias_count + header.enum_alias_count,           dl_ctx->enum_alias_count );
	dl_ctx->typedata_strings = dl_realloc_array( &dl_ctx->alloc, dl_ctx->typedata_strings, dl_ctx->typedata_strings_size + header.typeinfo_strings_size, dl_ctx->typedata_strings_size );

	memcpy( dl_ctx->type_ids         + dl_ctx->type_count,            lib_data + types_lookup_offset, sizeof( dl_typeid_t ) * header.type_count );
	memcpy( dl_ctx->enum_ids         + dl_ctx->enum_count,            lib_data + enums_lookup_offset, sizeof( dl_typeid_t ) * header.enum_count );
	memcpy( dl_ctx->type_descs       + dl_ctx->type_count,            lib_data + types_offset,        sizeof( dl_type_desc ) * header.type_count );
	memcpy( dl_ctx->enum_descs       + dl_ctx->enum_count,            lib_data + enums_offset,        sizeof( dl_enum_desc ) * header.enum_count );
	memcpy( dl_ctx->member_descs     + dl_ctx->member_count,          lib_data + members_offset,      sizeof( dl_member_desc ) * header.member_count );
	memcpy( dl_ctx->enum_value_descs + dl_ctx->enum_value_count,      lib_data + enum_values_offset,  sizeof( dl_enum_value_desc ) * header.enum_value_count );
	memcpy( dl_ctx->enum_alias_descs + dl_ctx->enum_alias_count,      lib_data + enum_aliases_offset, sizeof( dl_enum_alias_desc ) * header.enum_alias_count );
	memcpy( dl_ctx->typedata_strings + dl_ctx->typedata_strings_size, lib_data + typedata_strings_offset, header.typeinfo_strings_size );

	if( DL_ENDIAN_HOST == DL_ENDIAN_BIG )
	{
		for( unsigned int i = 0; i < header.type_count; ++i ) dl_ctx->type_ids[ dl_ctx->type_count + i ] = dl_swap_endian_uint32( dl_ctx->type_ids[ dl_ctx->type_count + i ] );
		for( unsigned int i = 0; i < header.enum_count; ++i ) dl_ctx->enum_ids[ dl_ctx->enum_count + i ] = dl_swap_endian_uint32( dl_ctx->enum_ids[ dl_ctx->enum_count + i ] );
		for( unsigned int i = 0; i < header.type_count; ++i )       dl_endian_swap_type_desc( dl_ctx->type_descs + dl_ctx->type_count + i );
		for( unsigned int i = 0; i < header.enum_count; ++i )       dl_endian_swap_enum_desc( dl_ctx->enum_descs + dl_ctx->enum_count + i );
		for( unsigned int i = 0; i < header.member_count; ++i )     dl_endian_swap_member_desc( dl_ctx->member_descs + dl_ctx->member_count + i );
		for( unsigned int i = 0; i < header.enum_value_count; ++i ) dl_endian_swap_enum_value_desc( dl_ctx->enum_value_descs + dl_ctx->enum_value_count + i );
	}

	uint32_t td_str_offset = (uint32_t)dl_ctx->typedata_strings_size;
	for( unsigned int i = 0; i < header.type_count; ++i )
	{
		dl_ctx->type_descs[ dl_ctx->type_count + i ].name += td_str_offset;
		if(dl_ctx->type_descs[ dl_ctx->type_count + i ].comment != UINT32_MAX)
			dl_ctx->type_descs[ dl_ctx->type_count + i ].comment += td_str_offset;
		dl_ctx->type_descs[ dl_ctx->type_count + i ].member_start += dl_ctx->member_count;
	}

	for( unsigned int i = 0; i < header.member_count; ++i )
		dl_ctx->member_descs[ dl_ctx->member_count + i ].name += td_str_offset;

	for( unsigned int i = 0; i < header.enum_count; ++i )
	{
		dl_ctx->enum_descs[ dl_ctx->enum_count + i ].name += td_str_offset;
		if(dl_ctx->enum_descs[ dl_ctx->enum_count + i ].comment != UINT32_MAX)
			dl_ctx->enum_descs[ dl_ctx->enum_count+ i ].comment += td_str_offset;
		dl_ctx->enum_descs[ dl_ctx->enum_count + i ].value_start += dl_ctx->enum_value_count;
		dl_ctx->enum_descs[ dl_ctx->enum_count + i ].alias_start += dl_ctx->enum_alias_count;
	}

	for( unsigned int i = 0; i < header.enum_alias_count; ++i )
	{
		dl_ctx->enum_alias_descs[ dl_ctx->enum_alias_count + i ].name += td_str_offset;
		dl_ctx->enum_alias_descs[ dl_ctx->enum_alias_count + i ].value_index += dl_ctx->enum_value_count;
	}

	for( unsigned int i = 0; i < header.enum_value_count; ++i )
	{
		dl_ctx->enum_value_descs[ dl_ctx->enum_value_count + i ].main_alias += dl_ctx->enum_alias_count;
	}

	dl_ctx->type_count            += header.type_count;
	dl_ctx->enum_count            += header.enum_count;
	dl_ctx->member_count          += header.member_count;
	dl_ctx->enum_value_count      += header.enum_value_count;
	dl_ctx->enum_alias_count      += header.enum_alias_count;
	dl_ctx->typedata_strings_size += header.typeinfo_strings_size;

	// we still need to keep the capacity around here, even as they are the same as the type-counts in
	// the case where we were to read a typelib from text into this ctx as that would do an incremental
	// grow of the capacity.
	// One day we might get rid of that by working with temp-buffers when reading text?
	dl_ctx->type_capacity         = dl_ctx->type_count;
	dl_ctx->enum_capacity         = dl_ctx->enum_count;
	dl_ctx->member_capacity       = dl_ctx->member_count;
	dl_ctx->enum_value_capacity   = dl_ctx->enum_value_count;
	dl_ctx->enum_alias_capacity   = dl_ctx->enum_alias_count;
	dl_ctx->typedata_strings_cap  = dl_ctx->typedata_strings_size;

	return dl_internal_load_type_library_defaults( dl_ctx, lib_data + defaults_offset, header.default_value_size );
}
