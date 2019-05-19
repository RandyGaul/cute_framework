/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include "dl_internal_util.h"
#include "dl_types.h"
#include "dl_binary_writer.h"
#include <dl/dl_txt.h>

struct dl_txt_unpack_ctx
{
	const uint8_t* packed_instance;
	int indent;
	struct
	{
		uintptr_t offset;
		dl_typeid_t tid;
	} ptrs[256];
	int ptrs_count;
	bool has_ptrs;
};

static void dl_txt_unpack_write_indent( dl_binary_writer* writer, dl_txt_unpack_ctx* unpack_ctx )
{
	for( int i = 0; i < unpack_ctx->indent; ++i )
		dl_binary_writer_write_uint8( writer, ' ' );
}

static void dl_txt_unpack_write_string( dl_binary_writer* writer, const char* str )
{
	dl_binary_writer_write_uint8( writer, '\"' );
	while( *str )
	{
		switch( *str )
		{
			case '\'': dl_binary_writer_write( writer, "\\\'", 2 ); break;
			case '\"': dl_binary_writer_write( writer, "\\\"", 2 ); break;
			case '\\': dl_binary_writer_write( writer, "\\\\", 2 ); break;
			case '\n': dl_binary_writer_write( writer, "\\n", 2 ); break;
			case '\r': dl_binary_writer_write( writer, "\\r", 2 ); break;
			case '\t': dl_binary_writer_write( writer, "\\t", 2 ); break;
			case '\b': dl_binary_writer_write( writer, "\\b", 2 ); break;
			case '\f': dl_binary_writer_write( writer, "\\f", 2 ); break;
			break;
			default:
				dl_binary_writer_write_uint8( writer, (uint8_t)*str );
		}
		++str;
	}
	dl_binary_writer_write_uint8( writer, '\"' );
}

static void dl_txt_unpack_write_string_or_null( dl_binary_writer* writer, dl_txt_unpack_ctx* unpack_ctx, uintptr_t offset )
{
	if( offset == (uintptr_t)-1 )
		dl_binary_writer_write( writer, "null", 4 );
	else
		dl_txt_unpack_write_string( writer, (const char*)&unpack_ctx->packed_instance[offset] );
}

static void dl_txt_unpack_int8( dl_binary_writer* writer, int8_t data )
{
	char buffer[256];
	int len = dl_internal_str_format( buffer, sizeof(buffer), "%d", data );
	dl_binary_writer_write( writer, buffer, (size_t)len );
}

static void dl_txt_unpack_int16( dl_binary_writer* writer, int16_t data )
{
	char buffer[256];
	int len = dl_internal_str_format( buffer, sizeof(buffer), "%d", data );
	dl_binary_writer_write( writer, buffer, (size_t)len );
}

static void dl_txt_unpack_int32( dl_binary_writer* writer, int32_t data )
{
	char buffer[256];
	int len = dl_internal_str_format( buffer, sizeof(buffer), "%d", data );
	dl_binary_writer_write( writer, buffer, (size_t)len );
}

static void dl_txt_unpack_int64( dl_binary_writer* writer, int64_t data )
{
	char buffer[256];
	int len = dl_internal_str_format( buffer, sizeof(buffer), DL_INT64_FMT_STR, data );
	dl_binary_writer_write( writer, buffer, (size_t)len );
}

static void dl_txt_unpack_uint8( dl_binary_writer* writer, uint8_t data )
{
	char buffer[256];
	int len = dl_internal_str_format( buffer, sizeof(buffer), "%u", data );
	dl_binary_writer_write( writer, buffer, (size_t)len );
}

static void dl_txt_unpack_uint16( dl_binary_writer* writer, uint16_t data )
{
	char buffer[256];
	int len = dl_internal_str_format( buffer, sizeof(buffer), "%u", data );
	dl_binary_writer_write( writer, buffer, (size_t)len );
}

static void dl_txt_unpack_uint32( dl_binary_writer* writer, uint32_t data )
{
	char buffer[256];
	int len = dl_internal_str_format( buffer, sizeof(buffer), "%u", data );
	dl_binary_writer_write( writer, buffer, (size_t)len );
}

static void dl_txt_unpack_uint64( dl_binary_writer* writer, const uint64_t data )
{
	char buffer[256];
	int len = dl_internal_str_format( buffer, sizeof(buffer), DL_UINT64_FMT_STR, data );
	dl_binary_writer_write( writer, buffer, (size_t)len );
}

// Writing fp32/fp64 with %.9g/%.17g for full "round-tripabillity", see
// https://randomascii.wordpress.com/2012/03/08/float-precisionfrom-zero-to-100-digits-2/

static void dl_txt_unpack_fp32( dl_binary_writer* writer, float data )
{
	char buffer[256];
	int len = dl_internal_str_format( buffer, sizeof(buffer), "%.9g", data );
	dl_binary_writer_write( writer, buffer, (size_t)len );
}

static void dl_txt_unpack_fp64( dl_binary_writer* writer, double data )
{
	char buffer[256];
	int len = dl_internal_str_format( buffer, sizeof(buffer), "%.17g", data );
	dl_binary_writer_write( writer, buffer, (size_t)len );
}

static void dl_txt_unpack_enum( dl_ctx_t dl_ctx, dl_binary_writer* writer, const dl_enum_desc* e, uint64_t value )
{
	for( unsigned int j = 0; j < e->value_count; ++j )
	{
		const dl_enum_value_desc* v = dl_get_enum_value( dl_ctx, e, j );
		if( v->value == value )
		{

			dl_txt_unpack_write_string( writer, dl_internal_enum_alias_name( dl_ctx, &dl_ctx->enum_alias_descs[v->main_alias] ) );
			return;
		}
	}
	DL_ASSERT(false, "failed to find enum value " DL_PINT_FMT_STR, value);
}

static void dl_txt_unpack_ptr( dl_binary_writer* writer, uintptr_t offset )
{
	if( offset == (uintptr_t)-1 )
	{
		dl_binary_writer_write( writer, "null", 4 );
	}
	else if( offset == 0 )
	{
		dl_binary_writer_write( writer, "\"__root\"", 8 );
	}
	else
	{
		char buffer[256];
		dl_internal_str_format( buffer, sizeof(buffer), "ptr_" DL_UINT64_FMT_STR, (uint64_t)offset );
		dl_txt_unpack_write_string( writer, buffer );
	}
}

static void dl_txt_unpack_struct( dl_ctx_t dl_ctx, dl_txt_unpack_ctx* unpack_ctx, dl_binary_writer* writer, const dl_type_desc* type, const uint8_t* struct_data );

static void dl_txt_unpack_array( dl_ctx_t dl_ctx,
								 dl_txt_unpack_ctx* unpack_ctx,
								 dl_binary_writer*  writer,
								 dl_type_storage_t  storage,
								 const uint8_t*     array_data,
								 uint32_t           array_count,
								 dl_typeid_t        tid )
{
	dl_binary_writer_write_uint8( writer, '[' );
	switch( storage )
	{
		case DL_TYPE_STORAGE_INT8:
		{
			int8_t* mem = (int8_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_int8( writer, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_int8( writer, mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_INT16:
		{
			int16_t* mem = (int16_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_int16( writer, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_int16( writer, mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_INT32:
		{
			int32_t* mem = (int32_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_int32( writer, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_int32( writer, mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_INT64:
		{
			int64_t* mem = (int64_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_int64( writer, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_int64( writer, mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_UINT8:
		{
			uint8_t* mem = (uint8_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_uint8( writer, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_uint8( writer, mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_UINT16:
		{
			uint16_t* mem = (uint16_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_uint16( writer, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_uint16( writer, mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_UINT32:
		{
			uint32_t* mem = (uint32_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_uint32( writer, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_uint32( writer, mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_UINT64:
		{
			uint64_t* mem = (uint64_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_uint64( writer, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_uint64( writer, mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_FP32:
		{
			float* mem = (float*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_fp32( writer, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_fp32( writer, mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_FP64:
		{
			double* mem = (double*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_fp64( writer, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_fp64( writer, mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_STR:
		{
			uintptr_t* mem = (uintptr_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_write_string_or_null( writer, unpack_ctx, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_write_string_or_null( writer, unpack_ctx, mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_PTR:
		{
			uintptr_t* mem = (uintptr_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_ptr( writer, mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );
			}
			dl_txt_unpack_ptr( writer, mem[array_count - 1] );
			unpack_ctx->has_ptrs = true;
			break;
		}
		case DL_TYPE_STORAGE_STRUCT:
		{
			const dl_type_desc* type = dl_internal_find_type( dl_ctx, tid );
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_struct( dl_ctx, unpack_ctx, writer, type, array_data + i * type->size[DL_PTR_SIZE_HOST] );
				dl_binary_writer_write( writer, ", ", 2 );

			}
			dl_txt_unpack_struct( dl_ctx, unpack_ctx, writer, type, array_data + (array_count - 1) * type->size[DL_PTR_SIZE_HOST] );
			break;
		}
		case DL_TYPE_STORAGE_ENUM_INT8:
		{
			const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, tid );
			DL_ASSERT( e != 0x0, "handle this error!");

			int8_t* mem = (int8_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );

			}
			dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_ENUM_UINT8:
		{
			const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, tid );
			DL_ASSERT( e != 0x0, "handle this error!");

			uint8_t* mem = (uint8_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );

			}
			dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_ENUM_INT16:
		{
			const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, tid );
			DL_ASSERT( e != 0x0, "handle this error!");

			int16_t* mem = (int16_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );

			}
			dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_ENUM_UINT16:
		{
			const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, tid );
			DL_ASSERT( e != 0x0, "handle this error!");

			uint16_t* mem = (uint16_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );

			}
			dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_ENUM_INT32:
		{
			const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, tid );
			DL_ASSERT( e != 0x0, "handle this error!");

			int32_t* mem = (int32_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );

			}
			dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_ENUM_UINT32:
		{
			const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, tid );
			DL_ASSERT( e != 0x0, "handle this error!");

			uint32_t* mem = (uint32_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );

			}
			dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_ENUM_INT64:
		{
			const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, tid );
			DL_ASSERT( e != 0x0, "handle this error!");

			int64_t* mem = (int64_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );

			}
			dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[array_count - 1] );
		}
		break;
		case DL_TYPE_STORAGE_ENUM_UINT64:
		{
			const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, tid );
			DL_ASSERT( e != 0x0, "handle this error!");

			uint64_t* mem = (uint64_t*)array_data;
			for( uint32_t i = 0; i < array_count - 1; ++i )
			{
				dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[i] );
				dl_binary_writer_write( writer, ", ", 2 );

			}
			dl_txt_unpack_enum( dl_ctx, writer, e, (uint64_t)mem[array_count - 1] );
		}
		break;
		default:
			DL_ASSERT( false );
	}
	dl_binary_writer_write_uint8( writer, ']' );
}

static void dl_txt_unpack_member( dl_ctx_t dl_ctx, dl_txt_unpack_ctx* unpack_ctx, dl_binary_writer* writer, const dl_member_desc* member, const uint8_t* member_data )
{
	dl_txt_unpack_write_indent( writer, unpack_ctx );
	dl_txt_unpack_write_string( writer, dl_internal_member_name( dl_ctx, member ) );
	dl_binary_writer_write( writer, " : ", 3 );

	switch( member->AtomType() )
	{
		case DL_TYPE_ATOM_POD:
		{
			switch( member->StorageType() )
			{
				case DL_TYPE_STORAGE_INT8:        dl_txt_unpack_int8  ( writer, *(int8_t* )member_data ); break;
				case DL_TYPE_STORAGE_INT16:       dl_txt_unpack_int16 ( writer, *(int16_t*)member_data ); break;
				case DL_TYPE_STORAGE_INT32:       dl_txt_unpack_int32 ( writer, *(int32_t*)member_data ); break;
				case DL_TYPE_STORAGE_INT64:       dl_txt_unpack_int64 ( writer, *(int64_t*)member_data ); break;
				case DL_TYPE_STORAGE_UINT8:       dl_txt_unpack_uint8 ( writer, *(uint8_t* )member_data ); break;
				case DL_TYPE_STORAGE_UINT16:      dl_txt_unpack_uint16( writer, *(uint16_t*)member_data ); break;
				case DL_TYPE_STORAGE_UINT32:      dl_txt_unpack_uint32( writer, *(uint32_t*)member_data ); break;
				case DL_TYPE_STORAGE_UINT64:      dl_txt_unpack_uint64( writer, *(uint64_t*)member_data ); break;
				case DL_TYPE_STORAGE_FP32:        dl_txt_unpack_fp32  ( writer, *(float*)member_data ); break;
				case DL_TYPE_STORAGE_FP64:        dl_txt_unpack_fp64  ( writer, *(double*)member_data ); break;
				case DL_TYPE_STORAGE_ENUM_INT8:   dl_txt_unpack_enum  ( dl_ctx, writer, dl_internal_find_enum( dl_ctx, member->type_id ), (uint64_t)*( int8_t*)  member_data ); break;
				case DL_TYPE_STORAGE_ENUM_UINT8:  dl_txt_unpack_enum  ( dl_ctx, writer, dl_internal_find_enum( dl_ctx, member->type_id ), (uint64_t)*(uint8_t*)  member_data ); break;
				case DL_TYPE_STORAGE_ENUM_INT16:  dl_txt_unpack_enum  ( dl_ctx, writer, dl_internal_find_enum( dl_ctx, member->type_id ), (uint64_t)*( int16_t*) member_data ); break;
				case DL_TYPE_STORAGE_ENUM_UINT16: dl_txt_unpack_enum  ( dl_ctx, writer, dl_internal_find_enum( dl_ctx, member->type_id ), (uint64_t)*(uint16_t*) member_data ); break;
				case DL_TYPE_STORAGE_ENUM_INT32:  dl_txt_unpack_enum  ( dl_ctx, writer, dl_internal_find_enum( dl_ctx, member->type_id ), (uint64_t)*( int32_t*) member_data ); break;
				case DL_TYPE_STORAGE_ENUM_UINT32: dl_txt_unpack_enum  ( dl_ctx, writer, dl_internal_find_enum( dl_ctx, member->type_id ), (uint64_t)*(uint32_t*) member_data ); break;
				case DL_TYPE_STORAGE_ENUM_INT64:  dl_txt_unpack_enum  ( dl_ctx, writer, dl_internal_find_enum( dl_ctx, member->type_id ), (uint64_t)*( int64_t*) member_data ); break;
				case DL_TYPE_STORAGE_ENUM_UINT64: dl_txt_unpack_enum  ( dl_ctx, writer, dl_internal_find_enum( dl_ctx, member->type_id ), (uint64_t)*(uint64_t*) member_data ); break;
				case DL_TYPE_STORAGE_STR:         dl_txt_unpack_write_string_or_null( writer, unpack_ctx, *(uintptr_t*)member_data ); break;
				case DL_TYPE_STORAGE_PTR:
				{
					dl_txt_unpack_ptr( writer, *(uintptr_t*)member_data );
					unpack_ctx->has_ptrs = true;
				}
				break;
				case DL_TYPE_STORAGE_STRUCT: dl_txt_unpack_struct( dl_ctx, unpack_ctx, writer, dl_internal_find_type( dl_ctx, member->type_id ), member_data ); break;
				default:
					DL_ASSERT(false);
			}
		}
		break;
		case DL_TYPE_ATOM_ARRAY:
		{
			uintptr_t offset = *(uintptr_t*)member_data;
			uint32_t  count  = *(uint32_t*)(member_data + sizeof(uintptr_t));
			if( offset == (uintptr_t)-1 )
				dl_binary_writer_write( writer, "[]", 2 );
			else
				dl_txt_unpack_array( dl_ctx, unpack_ctx, writer, member->StorageType(), &unpack_ctx->packed_instance[offset], count, member->type_id );
		}
		break;
		case DL_TYPE_ATOM_INLINE_ARRAY:
			dl_txt_unpack_array( dl_ctx, unpack_ctx, writer, member->StorageType(), member_data, member->inline_array_cnt(), member->type_id );
		break;
		case DL_TYPE_ATOM_BITFIELD:
		{
			uint64_t write_me  = 0;
			uint32_t bf_bits   = member->bitfield_bits();
			uint32_t bf_offset = dl_bf_offset( DL_ENDIAN_HOST, member->size[DL_PTR_SIZE_HOST], member->bitfield_offset(), bf_bits );

			switch( member->size[DL_PTR_SIZE_HOST] )
			{
				case 1: write_me = DL_EXTRACT_BITS( uint64_t( *(uint8_t*)member_data), uint64_t(bf_offset), uint64_t(bf_bits) ); break;
				case 2: write_me = DL_EXTRACT_BITS( uint64_t(*(uint16_t*)member_data), uint64_t(bf_offset), uint64_t(bf_bits) ); break;
				case 4: write_me = DL_EXTRACT_BITS( uint64_t(*(uint32_t*)member_data), uint64_t(bf_offset), uint64_t(bf_bits) ); break;
				case 8: write_me = DL_EXTRACT_BITS( uint64_t(*(uint64_t*)member_data), uint64_t(bf_offset), uint64_t(bf_bits) ); break;
				default:
					DL_ASSERT(false && "This should not happen!");
					break;
			}
			dl_txt_unpack_uint64( writer, write_me );
		}
		break;
		default:
			DL_ASSERT( false );
	}
}

static void dl_txt_unpack_write_subdata( dl_ctx_t dl_ctx, dl_txt_unpack_ctx* unpack_ctx, dl_binary_writer* writer, const dl_type_desc* type, const uint8_t* struct_data );

static void dl_txt_unpack_write_subdata_ptr( dl_ctx_t            dl_ctx,
											 dl_txt_unpack_ctx*  unpack_ctx,
											 dl_binary_writer*   writer,
											 const uint8_t*      ptrptr,
											 const dl_type_desc* sub_type )
{
	uintptr_t offset = *(uintptr_t*)( ptrptr );
	if( offset == (uintptr_t)-1 )
		return;
	if( offset == 0 )
		return;

	for( int i = 0; i < unpack_ctx->ptrs_count; ++i )
		if( unpack_ctx->ptrs[i].offset == offset )
			return;

	// TODO: overflow!
	unpack_ctx->ptrs[unpack_ctx->ptrs_count++].offset = offset;

	dl_txt_unpack_write_indent( writer, unpack_ctx );
	dl_txt_unpack_ptr( writer, offset );
	dl_binary_writer_write( writer, " : ", 3 );

	dl_txt_unpack_struct( dl_ctx, unpack_ctx, writer, sub_type, &unpack_ctx->packed_instance[offset] );

	// TODO: extra , at last elem =/
	dl_binary_writer_write( writer, ",\n", 2 );

	dl_txt_unpack_write_subdata( dl_ctx, unpack_ctx, writer, sub_type, &unpack_ctx->packed_instance[offset] );
}

static void dl_txt_unpack_write_subdata_ptr_array( dl_ctx_t            dl_ctx,
											 	   dl_txt_unpack_ctx*  unpack_ctx,
												   dl_binary_writer*   writer,
												   const uint8_t*      array,
												   uint32_t            array_count,
												   const dl_type_desc* sub_type )
{
	for( uint32_t element = 0; element < array_count; ++element )
		dl_txt_unpack_write_subdata_ptr( dl_ctx,
										 unpack_ctx,
										 writer,
										 array + sizeof(void*) * element,
										 sub_type );
}

static void dl_txt_unpack_write_subdata( dl_ctx_t dl_ctx, dl_txt_unpack_ctx* unpack_ctx, dl_binary_writer* writer, const dl_type_desc* type, const uint8_t* struct_data )
{
	for( uint32_t member_index = 0; member_index < type->member_count; ++member_index )
	{
		const dl_member_desc* member = dl_get_type_member( dl_ctx, type, member_index );
		const uint8_t* member_data = struct_data + member->offset[DL_PTR_SIZE_HOST];
		switch( member->AtomType() )
		{
			case DL_TYPE_ATOM_POD:
			{
				switch( member->StorageType() )
				{
					case DL_TYPE_STORAGE_PTR:
						dl_txt_unpack_write_subdata_ptr( dl_ctx,
														 unpack_ctx,
														 writer,
														 member_data,
														 dl_internal_find_type( dl_ctx, member->type_id ) );
					break;

					case DL_TYPE_STORAGE_STRUCT:
					{
						const dl_type_desc* subtype = dl_internal_find_type( dl_ctx, member->type_id );
						if( subtype->flags & DL_TYPE_FLAG_HAS_SUBDATA )
							dl_txt_unpack_write_subdata( dl_ctx,
														 unpack_ctx,
														 writer,
														 subtype,
														 member_data );
					}
					break;
					default:
						// ignore ...
						break;
				}
			}
			break;
			case DL_TYPE_ATOM_INLINE_ARRAY:
			{
				switch( member->StorageType() )
				{
					case DL_TYPE_STORAGE_PTR:
						dl_txt_unpack_write_subdata_ptr_array( dl_ctx,
															   unpack_ctx,
															   writer,
															   member_data,
															   member->inline_array_cnt(),
															   dl_internal_find_type( dl_ctx, member->type_id ) );
					break;
					default:
						DL_ASSERT(false);
				}
			}
			break;
			case DL_TYPE_ATOM_ARRAY:
			{
				switch( member->StorageType() )
				{
					case DL_TYPE_STORAGE_STRUCT:
					{
						const dl_type_desc* subtype = dl_internal_find_type( dl_ctx, member->type_id );
						if( subtype->flags & DL_TYPE_FLAG_HAS_SUBDATA )
						{
							uintptr_t array_offset = *(uintptr_t*)(member_data);
							uint32_t  array_count  = *(uint32_t*)(member_data + sizeof(uintptr_t));
							const uint8_t* array = unpack_ctx->packed_instance + array_offset;
							for( uint32_t i = 0; i < array_count; ++i )
								dl_txt_unpack_write_subdata( dl_ctx, unpack_ctx, writer, subtype, array + i * subtype->size[DL_PTR_SIZE_HOST] );
						}
					}
					/* fall through */
					case DL_TYPE_STORAGE_PTR:
					{
						uintptr_t array_offset = *(uintptr_t*)(member_data);
						uint32_t  array_count  = *(uint32_t*)(member_data + sizeof(uintptr_t) );
						const uint8_t* array = unpack_ctx->packed_instance + array_offset;
						dl_txt_unpack_write_subdata_ptr_array( dl_ctx,
															   unpack_ctx,
															   writer,
															   array,
															   array_count,
															   dl_internal_find_type( dl_ctx, member->type_id ) );
					}
					break;
					default:
						// ignore ...
						break;
				}
			}
			break;
			default:
				// ignore ...
				break;
		}
	}
}

static void dl_txt_unpack_struct( dl_ctx_t dl_ctx, dl_txt_unpack_ctx* unpack_ctx, dl_binary_writer* writer, const dl_type_desc* type, const uint8_t* struct_data )
{
	dl_binary_writer_write( writer, "{\n", 2 );

	unpack_ctx->indent += 2;
	if( type->flags & DL_TYPE_FLAG_IS_UNION )
	{
		// TODO: check if type is not set at all ...
		size_t type_offset = dl_internal_union_type_offset( dl_ctx, type, DL_PTR_SIZE_HOST );

		// find member index from union type ...
		uint32_t union_type = *((uint32_t*)(struct_data + type_offset));
		const dl_member_desc* member = dl_internal_find_member_desc_by_name_hash( dl_ctx, type, union_type );
		dl_txt_unpack_member( dl_ctx, unpack_ctx, writer, member, struct_data + member->offset[DL_PTR_SIZE_HOST] );
		dl_binary_writer_write( writer, "\n", 1 );
	}
	else
	{
		for( uint32_t member_index = 0; member_index < type->member_count; ++member_index )
		{
			const dl_member_desc* member = dl_get_type_member( dl_ctx, type, member_index );
			dl_txt_unpack_member( dl_ctx, unpack_ctx, writer, member, struct_data + member->offset[DL_PTR_SIZE_HOST] );
			if( member_index < type->member_count - 1 )
				dl_binary_writer_write( writer, ",\n", 2 );
			else
				dl_binary_writer_write( writer, "\n", 1 );
		}
	}

	if( struct_data == unpack_ctx->packed_instance )
	{
		if( unpack_ctx->has_ptrs )
		{
			unpack_ctx->indent += 2;

			dl_txt_unpack_write_indent( writer, unpack_ctx );
			dl_binary_writer_write( writer, ", \"__subdata\" : {\n", 18 );

			unpack_ctx->indent += 2;
			dl_txt_unpack_write_subdata( dl_ctx, unpack_ctx, writer, type, struct_data );
			unpack_ctx->indent -= 2;

			dl_txt_unpack_write_indent( writer, unpack_ctx );
			dl_binary_writer_write( writer, "}\n", 2 );

			unpack_ctx->indent -= 2;
		}
	}

	unpack_ctx->indent -= 2;

	dl_txt_unpack_write_indent( writer, unpack_ctx );
	dl_binary_writer_write_uint8( writer, '}' );
}

static dl_error_t dl_txt_unpack_root( dl_ctx_t dl_ctx, dl_txt_unpack_ctx* unpack_ctx, dl_binary_writer* writer, dl_typeid_t root_type )
{
	dl_binary_writer_write_uint8( writer, '{' );
	dl_binary_writer_write_uint8( writer, '\n' );

	const dl_type_desc* type = dl_internal_find_type(dl_ctx, root_type);
	if( type == 0x0 )
		return DL_ERROR_TYPE_NOT_FOUND; // could not find root-type!

	unpack_ctx->indent += 2;
	dl_txt_unpack_write_indent( writer, unpack_ctx );
	dl_txt_unpack_write_string( writer, dl_internal_type_name( dl_ctx, type ) );
	dl_binary_writer_write( writer, " : ", 3 );
	dl_txt_unpack_struct( dl_ctx, unpack_ctx, writer, type, unpack_ctx->packed_instance );
	unpack_ctx->indent -= 2;

	dl_binary_writer_write( writer, "\n}\0", 3 );
	return DL_ERROR_OK;
}

dl_error_t dl_txt_unpack( dl_ctx_t dl_ctx,                       dl_typeid_t type,
                          const unsigned char* packed_instance,  size_t      packed_instance_size,
                          char*                out_txt_instance, size_t      out_txt_instance_size,
                          size_t*              produced_bytes )
{
	dl_data_header* header = (dl_data_header*)packed_instance;

	if( packed_instance_size < sizeof(dl_data_header) ) return DL_ERROR_MALFORMED_DATA;
	if( header->id == DL_INSTANCE_ID_SWAPED )           return DL_ERROR_ENDIAN_MISMATCH;
	if( header->id != DL_INSTANCE_ID )                  return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION)         return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type )            return DL_ERROR_TYPE_MISMATCH;

	dl_binary_writer writer;
	dl_binary_writer_init( &writer,
						   (uint8_t*)out_txt_instance,
						   out_txt_instance_size,
						   false,
						   DL_ENDIAN_HOST,
						   DL_ENDIAN_HOST,
						   DL_PTR_SIZE_HOST );

	dl_txt_unpack_ctx unpackctx;
	unpackctx.packed_instance = packed_instance + sizeof(dl_data_header);
	unpackctx.indent = 0;
	unpackctx.ptrs_count = 0;
	unpackctx.has_ptrs = false;

	dl_txt_unpack_root( dl_ctx, &unpackctx, &writer, header->root_instance_type );
	if( produced_bytes )
		*produced_bytes = writer.needed_size;

	return DL_ERROR_OK;
};

dl_error_t dl_txt_unpack_calc_size( dl_ctx_t dl_ctx,                           dl_typeid_t type,
                                    const unsigned char* packed_instance,      size_t      packed_instance_size,
                                    size_t*              out_txt_instance_size )
{
	return dl_txt_unpack( dl_ctx, type, packed_instance, packed_instance_size, 0x0, 0, out_txt_instance_size );
}
