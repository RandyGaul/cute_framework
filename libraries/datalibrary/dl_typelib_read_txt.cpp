#include <dl/dl_typelib.h>
#include <dl/dl_txt.h>
#include "dl_internal_util.h"
#include "dl_types.h"
#include "dl_alloc.h"
#include "dl_txt_read.h"

#include <stdlib.h> // strtoul
#include <ctype.h>

template <typename T>
static inline T* dl_grow_array( dl_allocator* alloc, T* ptr, size_t* cap, size_t min_inc )
{
	size_t old_cap = *cap;
	size_t new_cap = ( ( old_cap < min_inc ) ? old_cap + min_inc : old_cap ) * 2;
	if( new_cap == 0 )
		new_cap = 8;
	*cap = new_cap;
	return (T*)dl_realloc( alloc, ptr, new_cap * sizeof( T ), old_cap * sizeof( T ) );
}

static uint32_t dl_alloc_string( dl_ctx_t ctx, dl_txt_read_substr* str )
{
	if( ctx->typedata_strings_cap - ctx->typedata_strings_size < (size_t)str->len + 2 )
	{
		ctx->typedata_strings = dl_grow_array( &ctx->alloc, ctx->typedata_strings, &ctx->typedata_strings_cap, (size_t)str->len + 2 );
	}
	uint32_t pos = (uint32_t)ctx->typedata_strings_size;
	memcpy( &ctx->typedata_strings[ pos ], str->str, (size_t)str->len );
	ctx->typedata_strings[ pos + (size_t)str->len ] = 0;
	ctx->typedata_strings_size += (size_t)str->len + 1;
	return pos;
}

static dl_type_desc* dl_alloc_type( dl_ctx_t ctx, dl_typeid_t tid )
{
	if( ctx->type_capacity <= ctx->type_count )
	{
		size_t cap = ctx->type_capacity;
		ctx->type_ids   = dl_grow_array( &ctx->alloc, ctx->type_ids, &cap, 0 );
		ctx->type_descs = dl_grow_array( &ctx->alloc, ctx->type_descs, &ctx->type_capacity, 0 );
	}

	unsigned int type_index = ctx->type_count;
	++ctx->type_count;

	ctx->type_ids[ type_index ] = tid;
	dl_type_desc* type = ctx->type_descs + type_index;
	memset( type, 0x0, sizeof( dl_type_desc ) );
	type->flags = DL_TYPE_FLAG_DEFAULT;
	type->member_start = ctx->member_count;
	type->member_count = 0;

	return type;
}

static dl_member_desc* dl_alloc_member( dl_ctx_t ctx )
{
	if( ctx->member_capacity <= ctx->member_count )
		ctx->member_descs = dl_grow_array( &ctx->alloc, ctx->member_descs, &ctx->member_capacity, 0 );

	unsigned int member_index = ctx->member_count;
	++ctx->member_count;

	dl_member_desc* member = ctx->member_descs + member_index;
	memset( member, 0x0, sizeof( dl_member_desc ) );
	member->default_value_offset = 0xFFFFFFFF;
	member->default_value_size = 0;
	member->comment = UINT32_MAX;
	return member;
}

static dl_enum_desc* dl_alloc_enum( dl_ctx_t ctx, dl_txt_read_substr* name )
{
	if( ctx->enum_capacity <= ctx->enum_count )
	{
		size_t cap = ctx->enum_capacity;
		ctx->enum_ids   = dl_grow_array( &ctx->alloc, ctx->enum_ids, &cap, 0 );
		ctx->enum_descs = dl_grow_array( &ctx->alloc, ctx->enum_descs, &ctx->enum_capacity, 0 );
	}

	unsigned int enum_index = ctx->enum_count;
	++ctx->enum_count;

	ctx->enum_ids[ enum_index ] = dl_internal_hash_buffer( (const uint8_t*)name->str, (size_t)name->len );

	dl_enum_desc* e = &ctx->enum_descs[enum_index];
	e->name = dl_alloc_string( ctx, name );
	e->value_start = ctx->enum_value_count;
	e->value_count = 0;
	e->alias_count = 0;
	e->alias_start = ctx->enum_alias_count;
	return e;
}

static dl_enum_value_desc* dl_alloc_enum_value( dl_ctx_t ctx )
{
	if( ctx->enum_value_capacity <= ctx->enum_value_count )
		ctx->enum_value_descs = dl_grow_array( &ctx->alloc, ctx->enum_value_descs, &ctx->enum_value_capacity, 0 );

	unsigned int value_index = ctx->enum_value_count;
	++ctx->enum_value_count;

	dl_enum_value_desc* value = ctx->enum_value_descs + value_index;
	value->main_alias = 0;

	return value;
}

static dl_enum_alias_desc* dl_alloc_enum_alias( dl_ctx_t ctx, dl_txt_read_substr* name )
{
	if( ctx->enum_alias_capacity <= ctx->enum_alias_count )
		ctx->enum_alias_descs = dl_grow_array( &ctx->alloc, ctx->enum_alias_descs, &ctx->enum_alias_capacity, 0 );

	unsigned int alias_index = ctx->enum_alias_count;
	++ctx->enum_alias_count;

	dl_enum_alias_desc* alias = &ctx->enum_alias_descs[ alias_index ];
	alias->value_index = 0xFFFFFFFF;
	alias->name = dl_alloc_string( ctx, name );
	return alias;
}

static void dl_set_member_size_and_align_from_builtin( dl_type_storage_t storage, dl_member_desc* member )
{
	switch( storage )
	{
		case DL_TYPE_STORAGE_STR:
		case DL_TYPE_STORAGE_PTR:
			member->set_size( 4, 8 );
			member->set_align( 4, 8 );
			break;
		default:
		{
			uint32_t size = (uint32_t)dl_pod_size(storage);
			member->set_size( size, size );
			member->set_align( size, size );
		}
	}
}

struct dl_builtin_type
{
	const char*       name;
	dl_type_storage_t type;
};

static const dl_builtin_type BUILTIN_TYPES[] = {
	{ "int8",   DL_TYPE_STORAGE_INT8 },
	{ "uint8",  DL_TYPE_STORAGE_UINT8 },
	{ "int16",  DL_TYPE_STORAGE_INT16 },
	{ "uint16", DL_TYPE_STORAGE_UINT16 },
	{ "int32",  DL_TYPE_STORAGE_INT32 },
	{ "uint32", DL_TYPE_STORAGE_UINT32 },
	{ "int64",  DL_TYPE_STORAGE_INT64 },
	{ "uint64", DL_TYPE_STORAGE_UINT64 },
	{ "fp32",   DL_TYPE_STORAGE_FP32 },
	{ "fp64",   DL_TYPE_STORAGE_FP64 },
	{ "string", DL_TYPE_STORAGE_STR },
};

static const dl_builtin_type* dl_find_builtin_type( const char* name )
{
	for( size_t i = 0; i < DL_ARRAY_LENGTH( BUILTIN_TYPES ); ++i )
	{
		const dl_builtin_type* builtin = &BUILTIN_TYPES[i];
		if( strcmp( name, builtin->name ) == 0 )
			return builtin;
	}
	return 0x0;
}

dl_type_t dl_make_type( dl_type_atom_t atom, dl_type_storage_t storage );

static void dl_load_txt_build_default_data( dl_ctx_t ctx, dl_txt_read_ctx* read_state, unsigned int member_index )
{
	if( ctx->member_descs[member_index].default_value_offset == 0xFFFFFFFF )
		return;

	// TODO: check that this is not outside the buffers
	dl_type_desc*   def_type   = dl_alloc_type( ctx, dl_internal_hash_string( "a_type_here" ) );
	dl_member_desc* def_member = dl_alloc_member( ctx );

	dl_member_desc* member = &ctx->member_descs[member_index];

	uint32_t def_start = member->default_value_offset;
	uint32_t def_len   = member->default_value_size;

	char def_buffer[2048]; // TODO: no hardcode =/

	// TODO: check that typename do not exist in the ctx!

	size_t name_start = ctx->typedata_strings_size;
	dl_txt_read_substr temp = { "a_type_here", 11 };
	def_type->name = dl_alloc_string( ctx, &temp );
	def_type->size[DL_PTR_SIZE_HOST]      = member->size[DL_PTR_SIZE_HOST];
	def_type->alignment[DL_PTR_SIZE_HOST] = member->alignment[DL_PTR_SIZE_HOST];
	def_type->member_count = 1;

	memcpy( def_member, member, sizeof( dl_member_desc ) );
	def_member->offset[0] = 0;
	def_member->offset[1] = 0;

	dl_internal_str_format( def_buffer, sizeof(def_buffer), "{\"a_type_here\":{\"%s\":%.*s}}", dl_internal_member_name( ctx, member ), (int)def_len, read_state->start + def_start );

	size_t prod_bytes;
	dl_error_t err;
	err = dl_txt_pack( ctx, def_buffer, 0x0, 0, &prod_bytes );
	if( err != DL_ERROR_OK )
		dl_txt_read_failed( ctx, read_state, DL_ERROR_INVALID_DEFAULT_VALUE, "failed to pack default-value for member \"%s\" with error \"%s\"",
															dl_internal_member_name( ctx, member ),
															dl_error_to_string( err ) );

	uint8_t* pack_buffer = (uint8_t*)dl_alloc( &ctx->alloc, prod_bytes );

	dl_txt_pack( ctx, def_buffer, pack_buffer, prod_bytes, 0x0 );

	// TODO: convert packed instance to typelib endian/ptrsize here!

	size_t inst_size = prod_bytes - sizeof( dl_data_header );

	ctx->default_data = (uint8_t*)dl_realloc( &ctx->alloc, ctx->default_data, ctx->default_data_size + inst_size, ctx->default_data_size );
	memcpy( ctx->default_data + ctx->default_data_size, pack_buffer + sizeof( dl_data_header ), inst_size );

	dl_free( &ctx->alloc, pack_buffer );

	member->default_value_offset = (uint32_t)ctx->default_data_size;
	member->default_value_size   = (uint32_t)inst_size;
	ctx->default_data_size += inst_size;

	--ctx->type_count;
	--ctx->member_count;
	ctx->typedata_strings_size = name_start;
}

static dl_member_desc* dl_load_txt_find_first_bitfield_member( dl_member_desc* start, dl_member_desc* end )
{
	while( start <= end )
	{
		if( start->AtomType() == DL_TYPE_ATOM_BITFIELD )
			return start;
		++start;
	}
	return 0x0;
}

static dl_member_desc* dl_load_txt_find_last_bitfield_member( dl_member_desc* start, dl_member_desc* end )
{
	while( start <= end )
	{
		if( start->AtomType() != DL_TYPE_ATOM_BITFIELD )
			return start - 1;
		++start;
	}
	return end;
}

static void dl_load_txt_fixup_bitfield_members( dl_ctx_t ctx, dl_type_desc* type )
{
	dl_member_desc* start = ctx->member_descs + type->member_start;
	dl_member_desc* end   = start + type->member_count - 1;

	while( true )
	{
		dl_member_desc* group_start = dl_load_txt_find_first_bitfield_member( start, end );
		if( group_start == 0x0 )
			return; // done!
		dl_member_desc* group_end = dl_load_txt_find_last_bitfield_member( group_start, end );

		unsigned int group_bits = 0;
		for( dl_member_desc* iter = group_start; iter <= group_end; ++iter )
		{
			iter->set_bitfield_offset( group_bits );
			group_bits += iter->bitfield_bits();
		}

		// TODO: handle higher bit-counts than 64!
		dl_type_storage_t storage = DL_TYPE_STORAGE_UINT8;
		if     ( group_bits <= 8  ) storage = DL_TYPE_STORAGE_UINT8;
		else if( group_bits <= 16 ) storage = DL_TYPE_STORAGE_UINT16;
		else if( group_bits <= 32 ) storage = DL_TYPE_STORAGE_UINT32;
		else if( group_bits <= 64 ) storage = DL_TYPE_STORAGE_UINT64;
		else
			DL_ASSERT( false );

		for( dl_member_desc* iter = group_start; iter <= group_end; ++iter )
		{
			iter->set_storage( storage );
			dl_set_member_size_and_align_from_builtin( storage, iter );
		}

		start = group_end + 1;
	}
}

static inline bool dl_internal_find_enum_value_from_name( dl_ctx_t ctx, const char* name, size_t name_len, uint64_t* value )
{
	for( unsigned int i = 0; i < ctx->enum_alias_count; ++i )
	{
		const char* alias_name = dl_internal_enum_alias_name( ctx, &ctx->enum_alias_descs[i] );
		if( strncmp( alias_name, name, name_len ) == 0 )
		{
			*value = ctx->enum_value_descs[ctx->enum_alias_descs[i].value_index].value;
			return true;
		}
	}
	return false;
}

static void dl_load_txt_calc_type_size_and_align( dl_ctx_t ctx, dl_txt_read_ctx* read_state, dl_type_desc* type )
{
	// ... is the type already processed ...
	if( type->size[0] > 0 )
		return;

	dl_load_txt_fixup_bitfield_members( ctx, type );

	uint32_t size[2]  = { 0, 0 };
	uint32_t align[2] = { type->alignment[DL_PTR_SIZE_32BIT], type->alignment[DL_PTR_SIZE_64BIT] };

	unsigned int mem_start = type->member_start;
	unsigned int mem_end   = type->member_start + type->member_count;

	dl_member_desc* bitfield_group_start = 0x0;

	for( unsigned int member_index = mem_start; member_index < mem_end; ++member_index )
	{
		dl_member_desc* member = ctx->member_descs + member_index;

		// If a member is marked as a struct it could also have been an enum that we didn't know about parse-time, patch it in that case.
		if( member->StorageType() == DL_TYPE_STORAGE_STRUCT )
		{
			if( const dl_enum_desc* edesc = dl_internal_find_enum( ctx, member->type_id ) )
				member->set_storage( edesc->storage );
		}

		dl_type_atom_t    atom    = member->AtomType();
		dl_type_storage_t storage = member->StorageType();

		switch( atom )
		{
			case DL_TYPE_ATOM_POD:
			case DL_TYPE_ATOM_INLINE_ARRAY:
			{
				if( atom == DL_TYPE_ATOM_INLINE_ARRAY )
				{
					if( member->inline_array_cnt() == 0 )
					{
						const char* enum_value_name = 0x0;
						uint32_t enum_value_name_len = 0;
						if( sizeof(void*) == 8 )
						{
							enum_value_name = (const char*)( ( (uint64_t)member->size[0] ) | (uint64_t)member->size[1] << 32 );
							enum_value_name_len = member->alignment[0];
						}
						else
						{
							enum_value_name = (const char*)(uint64_t)member->size[0];
							enum_value_name_len = member->alignment[0];
						}

						uint64_t val;
						if( !dl_internal_find_enum_value_from_name( ctx, enum_value_name, (size_t)enum_value_name_len, &val ) )
							dl_txt_read_failed( ctx, read_state, DL_ERROR_TXT_INVALID_ENUM_VALUE, "%s.%s is an inline array with size %.*s, but that enum value does not exist.",
												dl_internal_type_name( ctx, type ),
												dl_internal_member_name( ctx, member ),
												(int)enum_value_name_len,
												enum_value_name );

						member->set_inline_array_cnt( (uint32_t)val );
					}
				}

				if( storage == DL_TYPE_STORAGE_STRUCT )
				{
					const dl_type_desc* sub_type = dl_internal_find_type( ctx, member->type_id );
					if( sub_type == 0x0 )
						continue;

					dl_load_txt_calc_type_size_and_align( ctx, read_state, (dl_type_desc*)sub_type );
					member->copy_size( sub_type->size );
					member->copy_align( sub_type->alignment );
				}
				else
					dl_set_member_size_and_align_from_builtin( storage, member );

				if( atom == DL_TYPE_ATOM_INLINE_ARRAY )
				{
					member->size[DL_PTR_SIZE_32BIT] *= member->inline_array_cnt();
					member->size[DL_PTR_SIZE_64BIT] *= member->inline_array_cnt();
				}

				bitfield_group_start = 0x0;
			}
			break;
			case DL_TYPE_ATOM_ARRAY:
			{
				member->set_size( 8, 16 );
				member->set_align( 4, 8 );
			}
			break;
			case DL_TYPE_ATOM_BITFIELD:
			{
				if( bitfield_group_start )
				{
					member->offset[DL_PTR_SIZE_32BIT] = bitfield_group_start->offset[DL_PTR_SIZE_32BIT];
					member->offset[DL_PTR_SIZE_64BIT] = bitfield_group_start->offset[DL_PTR_SIZE_64BIT];
					continue;
				}
				bitfield_group_start = member;
			}
			break;
			default:
				bitfield_group_start = 0x0;
		}

		if( type->flags & DL_TYPE_FLAG_IS_UNION )
		{
			member->set_offset( 0, 0 );
			size[DL_PTR_SIZE_32BIT] = member->size[DL_PTR_SIZE_32BIT] > size[DL_PTR_SIZE_32BIT] ? member->size[DL_PTR_SIZE_32BIT] : size[DL_PTR_SIZE_32BIT];
			size[DL_PTR_SIZE_64BIT] = member->size[DL_PTR_SIZE_64BIT] > size[DL_PTR_SIZE_64BIT] ? member->size[DL_PTR_SIZE_64BIT] : size[DL_PTR_SIZE_64BIT];
		}
		else
		{
			member->offset[DL_PTR_SIZE_32BIT] = dl_internal_align_up( size[DL_PTR_SIZE_32BIT], member->alignment[DL_PTR_SIZE_32BIT] );
			member->offset[DL_PTR_SIZE_64BIT] = dl_internal_align_up( size[DL_PTR_SIZE_64BIT], member->alignment[DL_PTR_SIZE_64BIT] );
			size[DL_PTR_SIZE_32BIT] = member->offset[DL_PTR_SIZE_32BIT] + member->size[DL_PTR_SIZE_32BIT];
			size[DL_PTR_SIZE_64BIT] = member->offset[DL_PTR_SIZE_64BIT] + member->size[DL_PTR_SIZE_64BIT];
		}

		align[DL_PTR_SIZE_32BIT] = member->alignment[DL_PTR_SIZE_32BIT] > align[DL_PTR_SIZE_32BIT] ? member->alignment[DL_PTR_SIZE_32BIT] : align[DL_PTR_SIZE_32BIT];
		align[DL_PTR_SIZE_64BIT] = member->alignment[DL_PTR_SIZE_64BIT] > align[DL_PTR_SIZE_64BIT] ? member->alignment[DL_PTR_SIZE_64BIT] : align[DL_PTR_SIZE_64BIT];
	}

	if( type->flags & DL_TYPE_FLAG_IS_UNION )
	{
		// ... add size for the union type flag ...
		size[DL_PTR_SIZE_32BIT] = dl_internal_align_up( size[DL_PTR_SIZE_32BIT], align[DL_PTR_SIZE_32BIT] ) + (uint32_t)sizeof(uint32_t);
		size[DL_PTR_SIZE_64BIT] = dl_internal_align_up( size[DL_PTR_SIZE_64BIT], align[DL_PTR_SIZE_64BIT] ) + (uint32_t)sizeof(uint32_t);
	}

	type->size[DL_PTR_SIZE_32BIT] = dl_internal_align_up( size[DL_PTR_SIZE_32BIT], align[DL_PTR_SIZE_32BIT] );
	type->size[DL_PTR_SIZE_64BIT] = dl_internal_align_up( size[DL_PTR_SIZE_64BIT], align[DL_PTR_SIZE_64BIT] );
	type->alignment[DL_PTR_SIZE_32BIT] = align[DL_PTR_SIZE_32BIT];
	type->alignment[DL_PTR_SIZE_64BIT] = align[DL_PTR_SIZE_64BIT];
}

static bool dl_context_load_txt_type_has_subdata( dl_ctx_t ctx, dl_txt_read_ctx* read_state, const dl_type_desc* type )
{
	unsigned int mem_start = type->member_start;
	unsigned int mem_end   = type->member_start + type->member_count;

	// do the type have subdata?
	for( unsigned int member_index = mem_start; member_index < mem_end; ++member_index )
	{
		dl_member_desc* member = ctx->member_descs + member_index;
		dl_type_atom_t atom = member->AtomType();
		dl_type_storage_t storage = member->StorageType();

		switch( atom )
		{
			case DL_TYPE_ATOM_ARRAY:
				return true;
			default:
				break;
		}

		switch( storage )
		{
			case DL_TYPE_STORAGE_STR:
			case DL_TYPE_STORAGE_PTR:
				return true;
			case DL_TYPE_STORAGE_STRUCT:
			{
				const dl_type_desc* subtype = dl_internal_find_type( ctx, member->type_id );
				if( subtype == NULL )
					dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA, "Member is missing type id.");
				if( dl_context_load_txt_type_has_subdata( ctx, read_state, subtype ) )
					return true;
			}
			break;
			default:
				break;
		}
	}

	return false;
}

static void dl_context_load_txt_type_set_flags( dl_ctx_t ctx, dl_txt_read_ctx* read_state, dl_type_desc* type )
{
	if( dl_context_load_txt_type_has_subdata( ctx, read_state, type ) )
		type->flags |= (uint32_t)DL_TYPE_FLAG_HAS_SUBDATA;
}

const char* dl_txt_skip_map( const char* iter, const char* end );
const char* dl_txt_skip_string( const char* str, const char* end );

static inline uint32_t dl_txt_pack_eat_uint32( dl_ctx_t dl_ctx, dl_txt_read_ctx* read_state )
{
	return (uint32_t)dl_txt_pack_eat_strtoull(dl_ctx, read_state, UINT32_MAX, "uint32");
}

static dl_txt_read_substr dl_txt_eat_and_expect_string( dl_ctx_t ctx, dl_txt_read_ctx* read_state )
{
	dl_txt_eat_white( read_state );
	dl_txt_read_substr str = dl_txt_eat_string( read_state );
	if( str.str == 0x0 )
		dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA, "expected string" );
	return str;
}

static bool dl_txt_try_eat_char( dl_txt_read_ctx* read_state, char c )
{
	dl_txt_eat_white( read_state );
	if( *read_state->iter != c )
		return false;
	++read_state->iter;
	return true;
}

static uint64_t dl_context_load_txt_type_library_read_enum_value( dl_ctx_t ctx,
															      dl_type_storage_t   storage,
															      dl_txt_read_ctx*    read_state )
{
	switch(storage)
	{
		case DL_TYPE_STORAGE_ENUM_INT8:   return (uint64_t)dl_txt_pack_eat_strtoll (ctx, read_state, INT8_MIN,  INT8_MAX,  "int8" );
		case DL_TYPE_STORAGE_ENUM_INT16:  return (uint64_t)dl_txt_pack_eat_strtoll (ctx, read_state, INT16_MIN, INT16_MAX, "int16");
		case DL_TYPE_STORAGE_ENUM_INT32:  return (uint64_t)dl_txt_pack_eat_strtoll (ctx, read_state, INT32_MIN, INT32_MAX, "int32");
		case DL_TYPE_STORAGE_ENUM_INT64:  return (uint64_t)dl_txt_pack_eat_strtoll (ctx, read_state, INT64_MIN, INT64_MAX, "int64");
		case DL_TYPE_STORAGE_ENUM_UINT8:  return dl_txt_pack_eat_strtoull(ctx, read_state, UINT8_MAX,  "uint8" );
		case DL_TYPE_STORAGE_ENUM_UINT16: return dl_txt_pack_eat_strtoull(ctx, read_state, UINT16_MAX, "uint16");
		case DL_TYPE_STORAGE_ENUM_UINT32: return dl_txt_pack_eat_strtoull(ctx, read_state, UINT32_MAX, "uint32");
		case DL_TYPE_STORAGE_ENUM_UINT64: return dl_txt_pack_eat_strtoull(ctx, read_state, UINT64_MAX, "uint64");
		default:
			DL_ASSERT(false);
			return 0;
	}
}

static void dl_context_load_txt_type_library_read_enum_values( dl_ctx_t ctx,
															   dl_type_storage_t   storage,
															   dl_txt_read_ctx*    read_state,
															   dl_txt_read_substr* value_name )
{
	dl_enum_value_desc* value = dl_alloc_enum_value( ctx );

	// ... alloc an alias for the base name ...
	dl_enum_alias_desc* alias = dl_alloc_enum_alias( ctx, value_name );
	alias->value_index = (uint32_t)(value - ctx->enum_value_descs);
	value->main_alias  = (uint32_t)(alias - ctx->enum_alias_descs);

	if( *read_state->iter == '{' )
	{
		dl_txt_eat_char( ctx, read_state, '{' );
		bool value_set = false;
		do
		{
			dl_txt_read_substr key = dl_txt_eat_and_expect_string( ctx, read_state );

			dl_txt_eat_char( ctx, read_state, ':' );

			if( strncmp( "value", key.str, 5 ) == 0 )
			{
				value->value = dl_context_load_txt_type_library_read_enum_value( ctx, storage, read_state );
				value_set = true;
			}
			else if( strncmp( "aliases", key.str, 7 ) == 0 )
			{
				dl_txt_eat_char( ctx, read_state, '[' );
				do
				{
					dl_txt_read_substr alias_name = dl_txt_eat_and_expect_string( ctx, read_state );

					dl_enum_alias_desc* enum_alias = dl_alloc_enum_alias( ctx, &alias_name );
					enum_alias->value_index = (uint32_t)(value - ctx->enum_value_descs);

				} while( dl_txt_try_eat_char( read_state, ',' ) );
				dl_txt_eat_char( ctx, read_state, ']' );
			}
			else
				dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA, "unexpected key '%.*s' in type, valid keys are 'value', or 'aliases'", key.len, key.str );

		} while( dl_txt_try_eat_char( read_state, ',' ) );

		if( !value_set )
			dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA, "enum value is having aliases but is missing its value!" );
		dl_txt_eat_char( ctx, read_state, '}' );
	}
	else
	{
		value->value = dl_context_load_txt_type_library_read_enum_value( ctx, storage, read_state );
	}
}

static void dl_context_load_txt_type_library_find_enum_keys( dl_ctx_t ctx,
															 dl_txt_read_ctx* read_state,
															 dl_txt_read_substr* name,
															 dl_txt_read_substr* comment,
															 const char** values_iter,
															 const char** type_iter,
															 const char** end_iter,
															 bool*        is_extern)
{
	*values_iter = 0x0;
	*type_iter   = 0x0;
	*end_iter    = 0x0;
	*is_extern   = false;

	dl_txt_eat_char( ctx, read_state, '{' );
	do
	{
		dl_txt_read_substr key = dl_txt_eat_and_expect_string( ctx, read_state );
		dl_txt_eat_char( ctx, read_state, ':' );
		dl_txt_eat_white( read_state );

		if( strncmp( "values", key.str, 6 ) == 0 )
		{
			if(*values_iter)
				dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA, "'values' set twice on enum '%.*s'", name->len, name->str );
			*values_iter = read_state->iter;
			read_state->iter = dl_txt_skip_map(read_state->iter, read_state->end);
		}
		else if( strncmp( "type", key.str, 4 ) == 0 )
		{
			if(*type_iter)
				dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA, "'type' set twice on enum '%.*s'", name->len, name->str );
			*type_iter = read_state->iter;
			read_state->iter = dl_txt_skip_string(read_state->iter + 1, read_state->end) + 1; // TODO: fix haxx, skip_string is not eating ' or "
		}
		else if( strncmp( "extern", key.str, 6 ) == 0 )
		{
			*is_extern = dl_txt_eat_bool( read_state ) == 1;
		}
		else if( strncmp( "comment", key.str, 7 ) == 0 )
		{
			*comment = dl_txt_eat_and_expect_string( ctx, read_state );
		}
		else
			dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA, "unexpected key '%.*s' in type, valid keys are 'values', 'type' and 'extern'", key.len, key.str );

	} while( dl_txt_try_eat_char( read_state, ',') );
	dl_txt_eat_char( ctx, read_state, '}' );

	if(*values_iter == 0x0)
		dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA, "'values' is missing on enum '%.*s'", name->len, name->str );

	*end_iter = read_state->iter;
}

static void dl_context_load_txt_type_library_read_enum( dl_ctx_t ctx, dl_txt_read_ctx* read_state, dl_txt_read_substr* name )
{
	uint32_t value_start = ctx->enum_value_count;
	uint32_t alias_start = ctx->enum_alias_count;

	const char* values_iter;
	const char* type_iter;
	const char* end_iter;
	dl_txt_read_substr comment = { 0, 0 };
	bool        is_extern;
	dl_context_load_txt_type_library_find_enum_keys(ctx, read_state, name, &comment, &values_iter, &type_iter, &end_iter, &is_extern);

	dl_type_storage_t storage = DL_TYPE_STORAGE_CNT;
	if(type_iter)
	{
		read_state->iter = type_iter;

		storage = DL_TYPE_STORAGE_CNT;

		dl_txt_read_substr type_str = dl_txt_eat_and_expect_string( ctx, read_state );
		switch(type_str.len)
		{
			case 4:
			{
				if( strncmp("int8", type_str.str, 4) == 0 )
					storage = DL_TYPE_STORAGE_ENUM_INT8;
			}
			break;
			case 5:
			{
					 if( strncmp("int16", type_str.str, 5) == 0 ) storage = DL_TYPE_STORAGE_ENUM_INT16;
				else if( strncmp("int32", type_str.str, 5) == 0 ) storage = DL_TYPE_STORAGE_ENUM_INT32;
				else if( strncmp("int64", type_str.str, 5) == 0 ) storage = DL_TYPE_STORAGE_ENUM_INT64;
				else if( strncmp("uint8", type_str.str, 5) == 0 ) storage = DL_TYPE_STORAGE_ENUM_UINT8;
			}
			break;
			case 6:
			{
					 if( strncmp("uint16", type_str.str, 6) == 0 ) storage = DL_TYPE_STORAGE_ENUM_UINT16;
				else if( strncmp("uint32", type_str.str, 6) == 0 ) storage = DL_TYPE_STORAGE_ENUM_UINT32;
				else if( strncmp("uint64", type_str.str, 6) == 0 ) storage = DL_TYPE_STORAGE_ENUM_UINT64;
			}
			break;
			default:
			break;
		}

		if(storage == DL_TYPE_STORAGE_CNT)
			dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA,
								"'type' on enum '%.*s' set to '%.*s', valid types are 'int8', 'int16', 'int32', 'int64', 'uint8', 'uint16', 'uint32' and 'uint64'",
									name->len, name->str,
									type_str.len, type_str.str );
	}

	if(storage == DL_TYPE_STORAGE_CNT )
		storage = DL_TYPE_STORAGE_ENUM_UINT32; // default to uint32

	read_state->iter = values_iter;

	dl_txt_eat_char( ctx, read_state, '{' );
	do
	{
		dl_txt_read_substr value_name = dl_txt_eat_and_expect_string( ctx, read_state );

		dl_txt_eat_char( ctx, read_state, ':' );
		dl_txt_eat_white( read_state );

		dl_context_load_txt_type_library_read_enum_values( ctx, storage, read_state, &value_name );
	} while( dl_txt_try_eat_char( read_state, ',') );
	dl_txt_eat_char( ctx, read_state, '}' );

	read_state->iter = end_iter;

	// TODO: add test for missing enum value ...

	dl_enum_desc* edesc = dl_alloc_enum(ctx, name);
	edesc->flags       = is_extern ? (uint32_t)DL_TYPE_FLAG_IS_EXTERNAL : 0;
	edesc->storage     = storage;
	edesc->value_count = ctx->enum_value_count - value_start;
	edesc->value_start = value_start;
	edesc->alias_count = ctx->enum_alias_count - alias_start; /// number of aliases for this enum, always at least 1. Alias 0 is consider the "main name" of the value and need to be a valid c enum name.
	edesc->alias_start = alias_start; /// offset into alias list where aliases for this enum-value start.
	edesc->comment     = comment.len > 0 ? dl_alloc_string( ctx, &comment ) : UINT32_MAX;
}

static void dl_context_load_txt_type_library_read_enums( dl_ctx_t ctx, dl_txt_read_ctx* read_state )
{
	dl_txt_eat_char( ctx, read_state, '{' );
	if( dl_txt_try_eat_char( read_state, '}' ) )
		return;

	do
	{
		dl_txt_read_substr enum_name = dl_txt_eat_and_expect_string( ctx, read_state );
		dl_txt_eat_char( ctx, read_state, ':' );
		dl_context_load_txt_type_library_read_enum( ctx, read_state, &enum_name );

	} while( dl_txt_try_eat_char( read_state, ',') );

	dl_txt_eat_char( ctx, read_state, '}' );
}

static int dl_parse_type( dl_ctx_t ctx, dl_txt_read_substr* type, dl_member_desc* member, dl_txt_read_ctx* read_state )
{
    #define DL_PARSE_TYPE_VALID_FMT_STR "\nvalid formats, 'type', 'type*', 'type[count]', 'type[]', 'bitfield:bits'"

	// ... strip whitespace ...
	char   type_name[2048];
	size_t type_name_len = 0;
	DL_ASSERT( (size_t)type->len < DL_ARRAY_LENGTH( type_name ) );

	const char* iter = type->str;
	const char* end  = type->str + type->len;

	while( ( iter != end ) && ( isalnum( *iter ) || *iter == '_' )  )
	{
		type_name[type_name_len++] = *iter;
		++iter;
	}
	type_name[type_name_len] = '\0';

	bool is_ptr = false;
	bool is_array = false;
	bool is_inline_array = false;
	unsigned int inline_array_len = 0;
	const char* inline_array_enum_value = 0x0;
	size_t inline_array_enum_value_size = 0;

	if( iter != end )
	{
		if( *iter == '*' )
		{
			is_ptr = true;
			if( iter[1] == '[' )
				++iter;
		}
		switch( *iter )
		{
            case '[':
            {
                ++iter;
                if( *iter == ']' )
                    is_array = true;
                else
                {
                    char* next = 0x0;
                    inline_array_len = (unsigned int)strtoul( iter, &next, 0 );
                    if( iter == next )
                    {
                        // ... failed to parse inline array as number, try it as an enum ..
                        while( *next != ']' && ( isalnum(*next) || *next == '_' ) ) ++next;

                        if( *next != ']' )
                            dl_txt_read_failed( ctx, read_state, DL_ERROR_TXT_PARSE_ERROR, "%.*s is not a valid type", type->len, type->str );

                        inline_array_enum_value = iter;
                        inline_array_enum_value_size = (size_t)(next - iter);
                    }
                    else
                    {
                        if( *next != ']' )
                            dl_txt_read_failed( ctx, read_state, DL_ERROR_TXT_PARSE_ERROR, "%.*s is not a valid type", type->len, type->str );
                    }
                    iter = next + 1;
                    is_inline_array = true;
                }
            }
            break;

            case ':':
            {
                if(strcmp( "bitfield", type_name ) != 0)
                    dl_txt_read_failed( ctx, read_state, DL_ERROR_TXT_PARSE_ERROR,
                        "found char (':') when parsing type '%.*s', this is only valid for type 'bitfield'"
                        DL_PARSE_TYPE_VALID_FMT_STR,
                        type->len, type->str );

                ++iter;

                member->type = dl_make_type( DL_TYPE_ATOM_BITFIELD, DL_TYPE_STORAGE_UINT8 );
                member->type_id = 0;
                char* next = 0x0;
                unsigned int bits = (unsigned int)strtoul( iter, &next, 0 );
                if( iter == next || *next != '\"' )
                    dl_txt_read_failed( ctx, read_state, DL_ERROR_TXT_PARSE_ERROR, "bitfield has a bad format, should be \"bitfield:<num_bits>\"" );

                member->set_bitfield_bits( bits );

                // type etc?
                return 1;
            }
            break;
            case '*':
            {
                // ignore ...
            }
            break;

            default:
            {
                dl_txt_read_failed( ctx, read_state, DL_ERROR_TXT_PARSE_ERROR,
                    "invalid char ('%c') found when parsing type '%.*s'\n"
                    DL_PARSE_TYPE_VALID_FMT_STR,
                    *iter, type->len, type->str );
            }
		}
	}

	if( strcmp( "bitfield", type_name ) == 0 )
		dl_txt_read_failed( ctx, read_state, DL_ERROR_TXT_PARSE_ERROR, "bitfield has a bad format, should be \"bitfield:<num_bits>\"" );

	dl_type_atom_t atom = DL_TYPE_ATOM_POD;
	if( is_array ) atom = DL_TYPE_ATOM_ARRAY;
	if( is_inline_array ) atom = DL_TYPE_ATOM_INLINE_ARRAY;

	const dl_builtin_type* builtin = dl_find_builtin_type( type_name );

	if( builtin )
	{
		if( is_ptr )
			dl_txt_read_failed( ctx, read_state, DL_ERROR_TXT_PARSE_ERROR, "pointer to pod is not supported!" );

		member->type = dl_make_type( atom, builtin->type );
		member->type_id = 0;
		dl_set_member_size_and_align_from_builtin( builtin->type, member );
	}
	else
	{
		member->type = dl_make_type( atom, is_ptr ? DL_TYPE_STORAGE_PTR : DL_TYPE_STORAGE_STRUCT );
		member->type_id = dl_internal_hash_string( type_name );
	}

	if( is_inline_array )
	{
		member->set_inline_array_cnt( inline_array_len );
		if( inline_array_len == 0 )
		{
			// If the inline array size is an enum we have to lookup the size when we are sure that all enums are parsed, temporarily store pointer and string-length
			// in size/align of member.
			if( sizeof(void*) == 8 )
			{
				member->set_size( (uint32_t)( (uint64_t)inline_array_enum_value & 0xFFFFFFFF ), (uint32_t)( ( (uint64_t)inline_array_enum_value >> 32 ) & 0xFFFFFFFF ) );
				member->set_align( (uint32_t)(inline_array_enum_value_size & 0xFFFFFFFF ), 0 );
			}
			else
			{
				member->set_size( (uint32_t)((uint64_t)inline_array_enum_value & 0xFFFFFFFF), 0 );
				member->set_align( (uint32_t)(inline_array_enum_value_size & 0xFFFFFFFF ), 0 );
			}
		}
	}

    #undef DL_PARSE_TYPE_VALID_FMT_STR

	return 1;
}

static const char* dl_txt_read_skip_array( const char* start )
{
	const char* end = start;
	int depth = 1;
	while(depth > 0) // extract to skip_array()
	{
		++end;
		if(*end == '[')
			++depth;
		else if(*end == ']')
			--depth;
	}
	return end + 1;
}

static void dl_context_load_txt_type_library_read_member( dl_ctx_t ctx, dl_txt_read_ctx* read_state )
{
	dl_txt_eat_char( ctx, read_state, '{' );

	dl_txt_read_substr name = {0,0};
	dl_txt_read_substr type = {0,0};
	dl_txt_read_substr comment = {0,0};
	dl_txt_read_substr default_val = {0,0};

	bool is_const = true;
	bool verify = true;

	do
	{
		dl_txt_read_substr key = dl_txt_eat_and_expect_string( ctx, read_state );

		dl_txt_eat_char( ctx, read_state, ':' );
		if( strncmp( "name", key.str, 4 ) == 0 )
		{
			name = dl_txt_eat_and_expect_string( ctx, read_state );
		}
		else if( strncmp( "type", key.str, 4 ) == 0 )
		{
			type = dl_txt_eat_and_expect_string( ctx, read_state );
		}
		else if( strncmp( "comment", key.str, 7 ) == 0 )
		{
			comment = dl_txt_eat_and_expect_string( ctx, read_state );
		}
		else if( strncmp( "default", key.str, 7 ) == 0 )
		{
			dl_txt_eat_white( read_state );
			const char* start = read_state->iter;
			const char* end = 0x0;
			switch( *start )
			{
				case '{':
					end = dl_txt_skip_map( start, read_state->end );
					break;
				case '\"':
					end = dl_txt_skip_string( start + 1, read_state->end );
					++end;
					break;
				case '[':
					end = dl_txt_read_skip_array( start );
					break;
				default:
					end = start;
					while(*end != ',' && *end != '}') ++end;
			}
			default_val.str = start;
			default_val.len = (int)(end - start);
			read_state->iter = end;
		}
		else if( strncmp( "const", key.str, 5) == 0)
		{
			is_const = dl_txt_eat_bool( read_state ) == 1;
		}
		else if( strncmp( "verify", key.str, 6) == 0)
		{
			verify = dl_txt_eat_bool( read_state ) == 1;
		}
		else
			dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA, "unexpected key '%.*s' in type, valid keys are 'name', 'type', 'default', 'comment' or 'verify'", key.len, key.str );

	} while( dl_txt_try_eat_char( read_state, ',') );

	dl_member_desc* member = dl_alloc_member( ctx );
	if (name.len == 0)
		dl_txt_read_failed(ctx, read_state, DL_ERROR_MALFORMED_DATA, "No name on member in type.");
	member->name = dl_alloc_string( ctx, &name );
	member->comment = comment.len > 0 ? dl_alloc_string( ctx, &comment ) : UINT32_MAX;
	dl_parse_type( ctx, &type, member, read_state );

	if(default_val.str)
	{
		member->default_value_offset = (uint32_t)( default_val.str - read_state->start );
		member->default_value_size   = (uint32_t)default_val.len;
	}

	if(is_const)
		member->flags |= (uint32_t)DL_MEMBER_FLAG_IS_CONST;
	if(verify)
		member->flags |= (uint32_t)DL_MEMBER_FLAG_VERIFY_EXTERNAL_SIZE_OFFSET;

	dl_txt_eat_char( ctx, read_state, '}' );
}

static uint32_t dl_context_load_txt_type_library_read_members( dl_ctx_t dl_ctx, dl_txt_read_ctx* read_state )
{
	uint32_t member_count = 0;
	dl_txt_eat_char( dl_ctx, read_state, '[' );
	dl_txt_eat_white( read_state );
	if( *read_state->iter == ']' )
		dl_txt_read_failed( dl_ctx, read_state, DL_ERROR_TYPELIB_MISSING_MEMBERS_IN_TYPE, "types without members are not allowed" );

	do
	{
		dl_context_load_txt_type_library_read_member( dl_ctx, read_state );
		++member_count;
	} while( dl_txt_try_eat_char( read_state, ',') );

	dl_txt_eat_char( dl_ctx, read_state, ']' );
	return member_count;
}

static void dl_context_load_txt_type_library_read_type( dl_ctx_t ctx, dl_txt_read_ctx* read_state, dl_txt_read_substr* name, bool is_union )
{
	dl_txt_eat_char( ctx, read_state, '{' );
	uint32_t align = 0;
	bool is_extern = false;
	bool verify = true;
	uint32_t member_count = 0;
	uint32_t member_start = ctx->member_count;
	dl_txt_read_substr comment = {0,0};

	do
	{
		dl_txt_eat_white( read_state );
		dl_txt_read_substr key = dl_txt_eat_string( read_state );
		if( key.str == 0x0 )
			break;

		dl_txt_eat_char( ctx, read_state, ':' );
		if( strncmp( "members", key.str, 7 ) == 0 )
		{
			member_count = dl_context_load_txt_type_library_read_members( ctx, read_state );
		}
		else if( strncmp( "align", key.str, 5 ) == 0 )
		{
			align = dl_txt_pack_eat_uint32( ctx, read_state );
		}
		else if( strncmp( "extern", key.str, 6 ) == 0 )
		{
			is_extern = dl_txt_eat_bool( read_state ) == 1;
		}
		else if( strncmp( "verify", key.str, 6 ) == 0 )
		{
			verify = dl_txt_eat_bool( read_state ) == 1;
		}
		else if( strncmp( "comment", key.str, 7 ) == 0 )
		{
			comment = dl_txt_eat_and_expect_string( ctx, read_state );
		}
		else
			dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA,
								 "unexpected key '%.*s' in type, valid keys are 'members', 'align', 'comment', 'extern' or 'verify'",
								 key.len, key.str );
	} while( dl_txt_try_eat_char( read_state, ',') );

	dl_typeid_t tid = dl_internal_hash_buffer( (const uint8_t*)name->str, (size_t)name->len );

	if( member_count == 0 )
		dl_txt_read_failed( ctx, read_state, DL_ERROR_TYPELIB_MISSING_MEMBERS_IN_TYPE, "types without members are not allowed" );

	dl_type_desc* type = dl_alloc_type( ctx, tid );
	type->name = dl_alloc_string( ctx, name );
	type->flags = 0;
	type->size[ DL_PTR_SIZE_32BIT ] = 0;
	type->size[ DL_PTR_SIZE_64BIT ] = 0;
	type->alignment[ DL_PTR_SIZE_32BIT ] = align;
	type->alignment[ DL_PTR_SIZE_64BIT ] = align;
	type->member_count = member_count;
	type->member_start = member_start;

	type->comment = comment.len > 0 ? dl_alloc_string( ctx, &comment ) : UINT32_MAX;

	if( is_extern )
		type->flags |= (uint32_t)DL_TYPE_FLAG_IS_EXTERNAL;
	if( is_union )
		type->flags |= (uint32_t)DL_TYPE_FLAG_IS_UNION;
	if( verify )
		type->flags |= (uint32_t)DL_TYPE_FLAG_VERIFY_EXTERNAL_SIZE_ALIGN;

	dl_txt_eat_char( ctx, read_state, '}' );
}

static void dl_context_load_txt_type_library_read_types( dl_ctx_t ctx, dl_txt_read_ctx* read_state, bool is_union )
{
	dl_txt_eat_char( ctx, read_state, '{' );
	if( dl_txt_try_eat_char( read_state, '}' ) )
		return;

	do
	{
		dl_txt_read_substr type_name = dl_txt_eat_and_expect_string( ctx, read_state );

		dl_txt_eat_char( ctx, read_state, ':' );
		dl_context_load_txt_type_library_read_type( ctx, read_state, &type_name, is_union );

	} while( dl_txt_try_eat_char( read_state, ',') );

	dl_txt_eat_char( ctx, read_state, '}' );
}

static const dl_type_desc* dl_internal_member_owner( dl_ctx_t ctx, const dl_member_desc* member )
{
	uint32_t member_index = (uint32_t)(member - ctx->member_descs);
	for(uint32_t type_index = 0; type_index < ctx->type_count; ++type_index)
	{
		const dl_type_desc* type = ctx->type_descs + type_index;
		if( member_index >= type->member_start &&
			member_index < type->member_start + type->member_count)
			return type;
	}
	DL_ASSERT(false, "couldn't find owner-type of member '%s'", dl_internal_member_name(ctx, member));
	return 0x0;
}

static void dl_context_load_txt_type_library_inner( dl_ctx_t ctx, dl_txt_read_ctx* read_state )
{
#if defined(_MSC_VER )
#pragma warning(push)
#pragma warning(disable:4611)
#endif
	if( setjmp( read_state->jumpbuf ) == 0 )
#if defined(_MSC_VER )
#pragma warning(pop)
#endif
	{
		uint32_t type_start = ctx->type_count;
		uint32_t member_start = ctx->member_count;

		dl_txt_eat_char( ctx, read_state, '{' );

		do
		{
			dl_txt_read_substr key = dl_txt_eat_and_expect_string( ctx, read_state );
			dl_txt_eat_char( ctx, read_state, ':' );

			if( strncmp( "module", key.str, 6 ) == 0 )
			{
				dl_txt_read_substr module = dl_txt_eat_and_expect_string( ctx, read_state );
				(void)module;
			}
			else if( strncmp( "usercode", key.str, 8 ) == 0 )
			{
				dl_txt_read_substr usercode = dl_txt_eat_and_expect_string( ctx, read_state );
				(void)usercode;
			}
			else if( strncmp( "enums", key.str, 5 ) == 0 )
			{
				dl_context_load_txt_type_library_read_enums( ctx, read_state );
			}
			else if( strncmp( "unions", key.str, 6 ) == 0 )
			{
				dl_context_load_txt_type_library_read_types( ctx, read_state, true );
			}
			else if( strncmp( "types", key.str, 5 ) == 0 )
			{
				dl_context_load_txt_type_library_read_types( ctx, read_state, false );
			}
			else
				dl_txt_read_failed( ctx, read_state, DL_ERROR_MALFORMED_DATA, "unexpected key '%.*s' in type, valid keys are 'module', 'usercode', 'enums', 'unions' or 'types'", key.len, key.str );

		} while( dl_txt_try_eat_char( read_state, ',') );

		dl_txt_eat_char( ctx, read_state, '}' );

		for( unsigned int i = type_start; i < ctx->type_count; ++i )
			dl_load_txt_calc_type_size_and_align( ctx, read_state, ctx->type_descs + i );

		// fixup members
		for( uint32_t member_index = member_start; member_index < ctx->member_count; ++member_index )
		{
			dl_member_desc* member = ctx->member_descs + member_index;
			if( member->type_id )
			{
				const dl_enum_desc* enum_sub_type = dl_internal_find_enum( ctx, member->type_id );
				if( enum_sub_type )
				{
					// ... type was really an enum ...
					member->set_storage( enum_sub_type->storage );
				}
				else
				{
					const dl_type_desc* sub_type = dl_internal_find_type( ctx, member->type_id );
					if( sub_type == 0x0 )
					{
						const dl_type_desc* owner_type = dl_internal_member_owner( ctx, member );
						dl_txt_read_failed( ctx, read_state, DL_ERROR_TYPE_NOT_FOUND, 
											"couldn't find type for member '%s::%s'", 
											dl_internal_type_name( ctx, owner_type ),
											dl_internal_member_name( ctx, member ) );
					}
				}
			}
		}

		for( uint32_t member_index = member_start; member_index < ctx->member_count; ++member_index )
			dl_load_txt_build_default_data( ctx, read_state, member_index );

		for( unsigned int i = type_start; i < ctx->type_count; ++i )
			dl_context_load_txt_type_set_flags( ctx, read_state, ctx->type_descs + i );
	}
	else
	{
		dl_report_error_location( ctx, read_state->start, read_state->end, read_state->iter );
	}
}

dl_error_t dl_context_load_txt_type_library( dl_ctx_t ctx, const char* lib_data, size_t lib_data_size )
{
	(void)lib_data_size;

	dl_txt_read_ctx read_state;
	read_state.start = lib_data;
	read_state.end   = lib_data + lib_data_size;
	read_state.iter  = lib_data;
	read_state.err   = DL_ERROR_OK;

	dl_context_load_txt_type_library_inner( ctx, &read_state );

	return read_state.err;
}
