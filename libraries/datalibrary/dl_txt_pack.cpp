/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include "dl_types.h"
#include <dl/dl_txt.h>
#include "dl_binary_writer.h"
#include "dl_patch_ptr.h"
#include "dl_txt_read.h"

#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <limits>

#if defined(_MSC_VER)
// TODO: any better/faster way to do this, especially strtof?
#  if _MSC_VER < 1800 // strtof was defined first in MSVC2013
	float strtof(const char* str, char** endptr)
	{
		return (float)strtod( str, endptr );
	}
#  endif
#endif

inline double dl_strtod(const char* str, char** endptr)
{
	double res = strtod(str, endptr);
	if(str == *endptr)
	{
		float sign = 1.0f;
		switch(str[0])
		{
			case '-':
				sign  = -1.0f;
				++str;
				break;
			case '+':
				sign  = 1.0f;
				++str;
				break;
			default:
				break;
		}
#if defined(_MSC_VER) && _MSC_VER < 1800 // older versions of strtod() on msvc did not parse inf
		if(_strnicmp("infinity", str, 8) == 0)
		{
			*endptr = (char*)str + 8;
			return std::numeric_limits<double>::infinity() * sign;
		}
		if(_strnicmp("inf", str, 3) == 0)
		{
			*endptr = (char*)str + 3;
			return std::numeric_limits<double>::infinity() * sign;
		}
		if(_strnicmp("nan", str, 3) == 0)
		{
			*endptr = (char*)str + 3;
			return std::numeric_limits<double>::quiet_NaN() * sign;
		}
#endif
			if( tolower(str[1]) == 'a' &&
			    tolower(str[2]) == 'x' &&
			   !isalnum(str[3]) )
			{
				*endptr = (char*)str + 3;
				return DBL_MAX * sign;
			}
			if( tolower(str[1]) == 'i' &&
			    tolower(str[2]) == 'n' &&
			   !isalnum(str[3]))
			{
				*endptr = (char*)str + 3;
				return DBL_MIN * sign;
			}
	}
	return res;
}

inline float dl_strtof(const char* str, char** endptr)
{
#if defined(_MSC_VER) && _MSC_VER < 1800 // strtof was defined first in MSVC2013
	float res = (float)strtod(str, endptr);
#else
	float res = strtof(str, endptr);
#endif

	if(str == *endptr)
	{
		float sign = 1.0f;
		switch(str[0])
		{
			case '-':
				sign  = -1.0f;
				++str;
				break;
			case '+':
				sign  = 1.0f;
				++str;
				break;
			default:
				break;
		}
#if defined(_MSC_VER) && _MSC_VER < 1800 // older versions of strtod() on msvc did not parse inf
		if(_strnicmp("infinity", str, 8) == 0)
		{
			*endptr = (char*)str + 8;
			return std::numeric_limits<float>::infinity() * sign;
		}
		if(_strnicmp("inf", str, 3) == 0)
		{
			*endptr = (char*)str + 3;
			return std::numeric_limits<float>::infinity() * sign;
		}
		if(_strnicmp("nan", str, 3) == 0)
		{
			*endptr = (char*)str + 3;
			return std::numeric_limits<float>::quiet_NaN() * sign;
		}
#endif
			if( tolower(str[1]) == 'a' &&
			    tolower(str[2]) == 'x' &&
			   !isalnum(str[3]) )
			{
				*endptr = (char*)str + 3;
				return FLT_MAX * sign;
			}
			if( tolower(str[1]) == 'i' &&
			    tolower(str[2]) == 'n' &&
			   !isalnum(str[3]))
			{
				*endptr = (char*)str + 3;
				return FLT_MIN * sign;
			}
	}
	return res;
}

struct dl_txt_pack_ctx
{
	dl_txt_read_ctx read_ctx;
	dl_binary_writer* writer;
	const char* subdata_pos;
	int subdata_count;

	struct
	{
		dl_txt_read_substr name;
		const dl_type_desc* type;
		size_t patch_pos;
	} subdata[256];
};

static void dl_txt_pack_eat_and_write_int8( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	long long v = dl_txt_pack_eat_strtoll(dl_ctx, &packctx->read_ctx, INT8_MIN, INT8_MAX, "int8");
	dl_binary_writer_write_int8( packctx->writer, (int8_t)v );
}

static void dl_txt_pack_eat_and_write_int16( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	long long v = dl_txt_pack_eat_strtoll(dl_ctx, &packctx->read_ctx, INT16_MIN, INT16_MAX, "int16");
	dl_binary_writer_write_int16( packctx->writer, (int16_t)v );
}

static void dl_txt_pack_eat_and_write_int32( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	long long v = dl_txt_pack_eat_strtoll(dl_ctx, &packctx->read_ctx, INT32_MIN, INT32_MAX, "int32");
	dl_binary_writer_write_int32( packctx->writer, (int32_t)v );
}

static void dl_txt_pack_eat_and_write_int64( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	long long v = dl_txt_pack_eat_strtoll(dl_ctx, &packctx->read_ctx, INT64_MIN, INT64_MAX, "int64");
	dl_binary_writer_write_int64( packctx->writer, (int64_t)v );
}

static void dl_txt_pack_eat_and_write_uint8( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	unsigned long long v = dl_txt_pack_eat_strtoull(dl_ctx, &packctx->read_ctx, UINT8_MAX, "uint8");
	dl_binary_writer_write_uint8( packctx->writer, (uint8_t)v );
}

static void dl_txt_pack_eat_and_write_uint16( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	unsigned long long v = dl_txt_pack_eat_strtoull(dl_ctx, &packctx->read_ctx, UINT16_MAX, "uint16");
	dl_binary_writer_write_uint16( packctx->writer, (uint16_t)v );
}

static void dl_txt_pack_eat_and_write_uint32( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	unsigned long long v = dl_txt_pack_eat_strtoull(dl_ctx, &packctx->read_ctx, UINT32_MAX, "uint32");
	dl_binary_writer_write_uint32( packctx->writer, (uint32_t)v );
}

static void dl_txt_pack_eat_and_write_uint64( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	unsigned long long v = dl_txt_pack_eat_strtoull(dl_ctx, &packctx->read_ctx, UINT64_MAX, "uint64");
	dl_binary_writer_write_uint64( packctx->writer, v );
}

static void dl_txt_pack_eat_and_write_fp32( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	dl_txt_eat_white( &packctx->read_ctx );
	char* next = 0x0;
	float v = dl_strtof( packctx->read_ctx.iter, &next );
	if( packctx->read_ctx.iter == next )
		dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "expected a value of type 'fp32'" );
	packctx->read_ctx.iter = next;
	dl_binary_writer_write_fp32( packctx->writer, v );
}

static void dl_txt_pack_eat_and_write_fp64( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	dl_txt_eat_white( &packctx->read_ctx );
	char* next = 0x0;
	double v = dl_strtod( packctx->read_ctx.iter, &next );
	if( packctx->read_ctx.iter == next )
		dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "expected a value of type 'fp64'" );
	packctx->read_ctx.iter = next;
	dl_binary_writer_write_fp64( packctx->writer, v );
}

static bool dl_txt_pack_eat_and_write_null(dl_txt_pack_ctx* packctx)
{
	dl_txt_eat_white( &packctx->read_ctx );
	if( strncmp( packctx->read_ctx.iter, "null", 4 ) == 0 )
	{
		dl_binary_writer_write_ptr( packctx->writer, (uintptr_t)-1 );
		packctx->read_ctx.iter += 4;
		return true;
	}
	return false;
}

static void dl_txt_pack_eat_and_write_string( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	dl_txt_eat_white( &packctx->read_ctx );
	if( dl_txt_pack_eat_and_write_null(packctx) )
		return;

	dl_txt_read_substr str = dl_txt_eat_string( &packctx->read_ctx );
	if( str.str == 0x0 )
		dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "expected a value of type 'string' or 'null'" );

	size_t curr = dl_binary_writer_tell( packctx->writer );
	dl_binary_writer_seek_end( packctx->writer );
	size_t strpos = dl_binary_writer_tell( packctx->writer );
	for( int i = 0; i < str.len; ++i )
	{
		if( str.str[i] == '\\' )
		{
			++i;
			switch( str.str[i] )
			{
				case '\'':
				case '\"':
				case '\\':
					dl_binary_writer_write_uint8( packctx->writer, (uint8_t)str.str[i] );
					break;
				case 'n': dl_binary_writer_write_uint8( packctx->writer, '\n' ); break;
				case 'r': dl_binary_writer_write_uint8( packctx->writer, '\r' ); break;
				case 't': dl_binary_writer_write_uint8( packctx->writer, '\t' ); break;
				case 'b': dl_binary_writer_write_uint8( packctx->writer, '\b' ); break;
				case 'f': dl_binary_writer_write_uint8( packctx->writer, '\f' ); break;
					break;
				default:
					DL_ASSERT( false && "unhandled escape!" );
			}
		}
		else
			dl_binary_writer_write_uint8( packctx->writer, (uint8_t)str.str[i] );
	}
	dl_binary_writer_write_uint8( packctx->writer, '\0' );
	dl_binary_writer_seek_set( packctx->writer, curr );
	dl_binary_writer_write( packctx->writer, &strpos, sizeof(size_t) );
}

static void dl_txt_pack_eat_and_write_enum( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx, const dl_enum_desc* edesc )
{
	dl_txt_eat_white( &packctx->read_ctx );
	dl_txt_read_substr ename = dl_txt_eat_string( &packctx->read_ctx );
	if( ename.str == 0x0 )
		dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "expected string" );

	uint64_t enum_value;
	if( !dl_internal_find_enum_value( dl_ctx, edesc, ename.str, (size_t)ename.len, &enum_value ) )
		dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TXT_INVALID_ENUM_VALUE, "the enum \"%s\" has no member named \"%.*s\"", dl_internal_enum_name( dl_ctx, edesc ), ename.len, ename.str );

	switch(edesc->storage)
	{
		case DL_TYPE_STORAGE_ENUM_INT8:   dl_binary_writer_write_int8  ( packctx->writer, (int8_t)  enum_value ); break;
		case DL_TYPE_STORAGE_ENUM_INT16:  dl_binary_writer_write_int16 ( packctx->writer, (int16_t) enum_value ); break;
		case DL_TYPE_STORAGE_ENUM_INT32:  dl_binary_writer_write_int32 ( packctx->writer, (int32_t) enum_value ); break;
		case DL_TYPE_STORAGE_ENUM_INT64:  dl_binary_writer_write_int64 ( packctx->writer, (int64_t) enum_value ); break;
		case DL_TYPE_STORAGE_ENUM_UINT8:  dl_binary_writer_write_uint8 ( packctx->writer, (uint8_t) enum_value ); break;
		case DL_TYPE_STORAGE_ENUM_UINT16: dl_binary_writer_write_uint16( packctx->writer, (uint16_t)enum_value ); break;
		case DL_TYPE_STORAGE_ENUM_UINT32: dl_binary_writer_write_uint32( packctx->writer, (uint32_t)enum_value ); break;
		case DL_TYPE_STORAGE_ENUM_UINT64: dl_binary_writer_write_uint64( packctx->writer, (uint64_t)enum_value ); break;
		default:
			DL_ASSERT(false);
	}
	packctx->read_ctx.iter = ename.str + ename.len + 1;
}

static void dl_txt_pack_eat_and_write_ptr( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx, const dl_type_desc* type, size_t patch_pos )
{
	if( dl_txt_pack_eat_and_write_null(packctx) )
		return;

	dl_txt_read_substr ptr = dl_txt_eat_string( &packctx->read_ctx );
	if( ptr.str == 0x0 )
		dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TXT_INVALID_MEMBER_TYPE, "expected string" );

	if( packctx->subdata_count == DL_ARRAY_LENGTH( packctx->subdata ) )
		dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "to many pointers! ( better error here! )" );

	packctx->subdata[packctx->subdata_count].name = ptr;
	packctx->subdata[packctx->subdata_count].type = type;
	packctx->subdata[packctx->subdata_count].patch_pos = patch_pos;
	++packctx->subdata_count;
}

static void dl_txt_pack_validate_c_symbol_key( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx, dl_txt_read_substr symbol )
{
	for(int i = 0; i < symbol.len; ++i)
	{
		if(!isalnum(symbol.str[i]) || symbol.str[i] == '_')
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TXT_PARSE_ERROR, "found a non-valid key \"%.*s\" in data, did you miss a string-terminator? (\" or \')", symbol.len, symbol.str );
	}
}

static void dl_txt_pack_eat_and_write_struct( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx, const dl_type_desc* type );

static void dl_txt_pack_eat_and_write_array( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx, const dl_member_desc* member, uint32_t array_length )
{
	switch( member->StorageType() )
	{
		case DL_TYPE_STORAGE_INT8:
		{
			for( uint32_t i = 0; i < array_length - 1; ++i )
			{
				dl_txt_pack_eat_and_write_int8( dl_ctx, packctx );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_int8( dl_ctx, packctx );
		}
		break;
		case DL_TYPE_STORAGE_INT16:
		{
			for( uint32_t i = 0; i < array_length - 1; ++i )
			{
				dl_txt_pack_eat_and_write_int16( dl_ctx, packctx );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_int16( dl_ctx, packctx );
		}
		break;
		case DL_TYPE_STORAGE_INT32:
		{
			for( uint32_t i = 0; i < array_length -1; ++i )
			{
				dl_txt_pack_eat_and_write_int32( dl_ctx, packctx );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_int32( dl_ctx, packctx );
		}
		break;
		case DL_TYPE_STORAGE_INT64:
		{
			for( uint32_t i = 0; i < array_length - 1; ++i )
			{
				dl_txt_pack_eat_and_write_int64( dl_ctx, packctx );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_int64( dl_ctx, packctx );
		}
		break;
		case DL_TYPE_STORAGE_UINT8:
		{
			for( uint32_t i = 0; i < array_length - 1; ++i )
			{
				dl_txt_pack_eat_and_write_uint8( dl_ctx, packctx );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_uint8( dl_ctx, packctx );
		}
		break;
		case DL_TYPE_STORAGE_UINT16:
		{
			for( uint32_t i = 0; i < array_length - 1; ++i )
			{
				dl_txt_pack_eat_and_write_uint16( dl_ctx, packctx );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_uint16( dl_ctx, packctx );
		}
		break;
		case DL_TYPE_STORAGE_UINT32:
		{
			for( uint32_t i = 0; i < array_length - 1; ++i )
			{
				dl_txt_pack_eat_and_write_uint32( dl_ctx, packctx );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_uint32( dl_ctx, packctx );
		}
		break;
		case DL_TYPE_STORAGE_UINT64:
		{
			for( uint32_t i = 0; i < array_length - 1; ++i )
			{
				dl_txt_pack_eat_and_write_uint64( dl_ctx, packctx );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_uint64( dl_ctx, packctx );
		}
		break;
		case DL_TYPE_STORAGE_FP32:
		{
			for( uint32_t i = 0; i < array_length - 1; ++i )
			{
				dl_txt_pack_eat_and_write_fp32( dl_ctx, packctx );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_fp32( dl_ctx, packctx );
		}
		break;
		case DL_TYPE_STORAGE_FP64:
		{
			for( uint32_t i = 0; i < array_length - 1; ++i )
			{
				dl_txt_pack_eat_and_write_fp64( dl_ctx, packctx );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_fp64( dl_ctx, packctx );
		}
		break;
		case DL_TYPE_STORAGE_STR:
		{
			for( uint32_t i = 0; i < array_length - 1; ++i )
			{
				dl_txt_pack_eat_and_write_string( dl_ctx, packctx );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_string( dl_ctx, packctx );
		}
		break;
		case DL_TYPE_STORAGE_PTR:
		{
			const dl_type_desc* type = dl_internal_find_type( dl_ctx, member->type_id );
			size_t array_pos = dl_binary_writer_tell( packctx->writer );
			for( uint32_t i = 0; i < array_length - 1; ++i )
			{
				dl_txt_pack_eat_and_write_ptr( dl_ctx, packctx, type, array_pos + i * sizeof(void*) );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_ptr( dl_ctx, packctx, type, array_pos + ( array_length - 1 ) * sizeof(void*) );
		}
		break;
		case DL_TYPE_STORAGE_STRUCT:
		{
			const dl_type_desc* type = dl_internal_find_type( dl_ctx, member->type_id );
			size_t array_pos = dl_binary_writer_tell( packctx->writer ); // TODO: this seek/set dance will only be needed if type has subptrs, optimize by making different code-paths?
			for( uint32_t i = 0; i < array_length -1; ++i )
			{
				dl_binary_writer_seek_set( packctx->writer, array_pos + i * type->size[DL_PTR_SIZE_HOST] );
				dl_txt_pack_eat_and_write_struct( dl_ctx, packctx, type );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_binary_writer_seek_set( packctx->writer, array_pos + (array_length - 1) * type->size[DL_PTR_SIZE_HOST] );
			dl_txt_pack_eat_and_write_struct( dl_ctx, packctx, type );
		}
		break;
		case DL_TYPE_STORAGE_ENUM_INT8:
		case DL_TYPE_STORAGE_ENUM_INT16:
		case DL_TYPE_STORAGE_ENUM_INT32:
		case DL_TYPE_STORAGE_ENUM_INT64:
		case DL_TYPE_STORAGE_ENUM_UINT8:
		case DL_TYPE_STORAGE_ENUM_UINT16:
		case DL_TYPE_STORAGE_ENUM_UINT32:
		case DL_TYPE_STORAGE_ENUM_UINT64:
		{
			const dl_enum_desc* edesc = dl_internal_find_enum( dl_ctx, member->type_id );
			if( edesc == 0x0 )
				dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TYPE_NOT_FOUND, "couldn't find enum-type of <type_name_here>.%s", dl_internal_member_name( dl_ctx, member ) );

			for( uint32_t i = 0; i < array_length -1; ++i )
			{
				dl_txt_pack_eat_and_write_enum( dl_ctx, packctx, edesc );
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
			}
			dl_txt_pack_eat_and_write_enum( dl_ctx, packctx, edesc );
		}
		break;
		default:
			DL_ASSERT(false);
			break;
	}

    // handle trailing ,
    dl_txt_eat_white( &packctx->read_ctx );
    if(*packctx->read_ctx.iter == ',')
        ++packctx->read_ctx.iter;
    dl_txt_eat_white( &packctx->read_ctx );
}

static void dl_txt_pack_array_item_size_align( dl_ctx_t dl_ctx,
											   const dl_member_desc* member,
											   size_t* size,
											   size_t* align )
{
	// TODO: store this in typelib?
	switch( member->StorageType() )
	{
		case DL_TYPE_STORAGE_STRUCT:
		{
			const dl_type_desc* type = dl_internal_find_type( dl_ctx, member->type_id );
			*size = type->size[DL_PTR_SIZE_HOST];
			*align = type->alignment[DL_PTR_SIZE_HOST];
			break;
		}
		default:
			*size = dl_pod_size( member->StorageType() );
			*align = *size;
			break;
	}
}

const char* dl_txt_skip_array( const char* iter, const char* end )
{
	iter = dl_txt_skip_white( iter, end );
	if( *iter != '[' )
		return "\0";
	++iter;

	int depth = 1;
	while( iter != end && depth > 0 )
	{
		iter = dl_txt_skip_white( iter, end );
		switch(*iter)
		{
			case 0x0: return "\0";
			case '[': ++depth; break;
			case ']': --depth; break;
			default: break;
		}
		++iter;
	}

	return iter;
}

const char* dl_txt_skip_map( const char* iter, const char* end )
{
	iter = dl_txt_skip_white( iter, end );
	if( *iter != '{' )
		return "\0";
	++iter;

	int depth = 1;
	while( iter != end && depth > 0 )
	{
		iter = dl_txt_skip_white( iter, end );
		switch(*iter)
		{
			case 0x0: return "\0";
			case '{': ++depth; break;
			case '}': --depth; break;
			default: break;
		}
		++iter;
	}

	return iter;
}

const char* dl_txt_skip_string( const char* str, const char* end )
{
	while( str != end && *str != '\"' )
	{
		if( *str == '\\' )
		{
			++str;
			if( str == end )
				return 0x0;
			++str;
			if( str == end )
				return 0x0;

			if( *str == '\"' )
				++str;
		}
		++str;
	}
	return str;
}

static uint32_t dl_txt_pack_find_array_length( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx, const dl_member_desc* member )
{
	const char* iter = packctx->read_ctx.iter;
	const char* end  = packctx->read_ctx.end;

	iter = dl_txt_skip_white( iter, end );
	if( *iter == ']' )
		return 0;

	switch( member->StorageType() )
	{
		case DL_TYPE_STORAGE_INT8: // TODO: for pods there are no need to check array length!
		case DL_TYPE_STORAGE_INT16:
		case DL_TYPE_STORAGE_INT32:
		case DL_TYPE_STORAGE_INT64:
		case DL_TYPE_STORAGE_UINT8:
		case DL_TYPE_STORAGE_UINT16:
		case DL_TYPE_STORAGE_UINT32:
		case DL_TYPE_STORAGE_UINT64:
		case DL_TYPE_STORAGE_FP32:
		case DL_TYPE_STORAGE_FP64:
		case DL_TYPE_STORAGE_PTR:
		case DL_TYPE_STORAGE_ENUM_INT8:
		case DL_TYPE_STORAGE_ENUM_INT16:
		case DL_TYPE_STORAGE_ENUM_INT32:
		case DL_TYPE_STORAGE_ENUM_INT64:
		case DL_TYPE_STORAGE_ENUM_UINT8:
		case DL_TYPE_STORAGE_ENUM_UINT16:
		case DL_TYPE_STORAGE_ENUM_UINT32:
		case DL_TYPE_STORAGE_ENUM_UINT64:
		 // TODO: bug, but in typelib build, there can't be any , in an enum-string.
		{
            bool last_was_comma = false;
			uint32_t array_length = 1;
			while(true)
			{
				iter = dl_txt_skip_white( iter, end );
				switch( *iter )
				{
					case ',':
						++array_length;
						break;
					case '\0':
					case ']':
						return last_was_comma ? array_length - 1 : array_length;
                    // TODO: I guess one can fool this parser by adding a ] or , in a comment at "the right place(tm)"
				}
                last_was_comma = *iter == ',';
				++iter;
			}
		}
		break;
		case DL_TYPE_STORAGE_STR:
		{
            bool last_was_comma = false;
			uint32_t array_length = 1;
			while(true)
			{
				iter = dl_txt_skip_white( iter, end );
				switch( *iter )
				{
					case ',':
						++array_length;
						break;
					case '"':
						iter = dl_txt_skip_string( ++iter, end );
						break;
					case '\0':
					case ']':
						return last_was_comma ? array_length - 1 : array_length;
                    // TODO: I guess one can fool this parser by adding a ] or , in a comment at "the right place(tm)"
				}
                last_was_comma = *iter == ',';
				++iter;
			}
		}
		break;
		case DL_TYPE_STORAGE_STRUCT:
		{
			bool last_was_comma = false;
			uint32_t array_length = 1;
			while(true)
			{
				iter = dl_txt_skip_white(iter, end);
				switch( *iter )
				{
					case ',':
						++array_length;
						++iter;
						last_was_comma = true;
						break;
					case '{':
						last_was_comma = false;
						iter = dl_txt_skip_map(iter, end);
						break;
					case '[':
						last_was_comma = false;
						iter = dl_txt_skip_array(iter, end);
						break;
					case '\0':
					case ']':
						return last_was_comma ? array_length - 1 : array_length;
					default:
						dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TXT_PARSE_ERROR,
									"Invalid txt-format, are you missing an '}' or an ']'?");
					// TODO: I guess one can fool this parser by adding a ] or , in a comment at "the right place(tm)"
				}
			}
		}
		break;
		default:
			DL_ASSERT(false);
	}
	return (uint32_t)-1;
}

static void dl_txt_pack_write_default_value( dl_ctx_t              dl_ctx,
											 dl_txt_pack_ctx*      packctx,
											 const dl_member_desc* member,
											 size_t                member_pos )
{
	uint8_t* member_default_value = dl_ctx->default_data + member->default_value_offset;

	uint32_t member_size = member->size[DL_PTR_SIZE_HOST];

	// ... handle bitfields differently since they take up sub-parts of bytes.
	if(member->AtomType() == DL_TYPE_ATOM_BITFIELD)
	{
		uint32_t bf_bits = member->bitfield_bits();
		uint32_t bf_offset = member->bitfield_offset();

		dl_binary_writer_seek_set( packctx->writer, member_pos );

		uint64_t current_data = 0;
		switch( member->StorageType() )
		{
			case DL_TYPE_STORAGE_UINT8:  current_data = (uint64_t)dl_binary_writer_read_uint8 ( packctx->writer ); break;
			case DL_TYPE_STORAGE_UINT16: current_data = (uint64_t)dl_binary_writer_read_uint16( packctx->writer ); break;
			case DL_TYPE_STORAGE_UINT32: current_data = (uint64_t)dl_binary_writer_read_uint32( packctx->writer ); break;
			case DL_TYPE_STORAGE_UINT64: current_data = dl_binary_writer_read_uint64( packctx->writer ); break;
			default:
				DL_ASSERT( false && "This should not happen!" );
				break;
		}

		uint64_t default_value = 0;

		switch( member->default_value_size )
		{
			case 1: default_value = (uint64_t)*member_default_value; break;
			case 2: default_value = (uint64_t)*(uint16_t*)member_default_value; break;
			case 3: default_value = (uint64_t)*(uint32_t*)member_default_value; break;
			case 4: default_value = (uint64_t)*(uint64_t*)member_default_value; break;
			default:
				DL_ASSERT( false && "This should not happen!" );
				break;
		}
		uint64_t default_value_extracted = DL_EXTRACT_BITS(default_value, dl_bf_offset( DL_ENDIAN_HOST, sizeof(uint8_t), bf_offset, bf_bits ), bf_bits);
		uint64_t to_store = DL_INSERT_BITS( current_data, default_value_extracted, dl_bf_offset( DL_ENDIAN_HOST, sizeof(uint8_t), bf_offset, bf_bits ), bf_bits );
		dl_binary_writer_seek_set( packctx->writer, member_pos );

		switch( member->StorageType() )
		{
			case DL_TYPE_STORAGE_UINT8:  dl_binary_writer_write_uint8 ( packctx->writer,  (uint8_t)to_store ); break;
			case DL_TYPE_STORAGE_UINT16: dl_binary_writer_write_uint16( packctx->writer, (uint16_t)to_store ); break;
			case DL_TYPE_STORAGE_UINT32: dl_binary_writer_write_uint32( packctx->writer, (uint32_t)to_store ); break;
			case DL_TYPE_STORAGE_UINT64: dl_binary_writer_write_uint64( packctx->writer, (uint64_t)to_store ); break;
			default:
				DL_ASSERT( false && "This should not happen!" );
				break;
		}

	}
	else
	{
		dl_binary_writer_seek_set( packctx->writer, member_pos );
		dl_binary_writer_write( packctx->writer, member_default_value, member->size[DL_PTR_SIZE_HOST] );
	}

	if( member_size != member->default_value_size )
	{
		uint8_t* subdata = member_default_value + member_size;
		// ... sub ptrs, copy and patch ...
		dl_binary_writer_seek_end( packctx->writer );
		uintptr_t subdata_pos = dl_binary_writer_tell( packctx->writer );

		dl_binary_writer_write( packctx->writer, subdata, member->default_value_size - member_size );

		uint8_t* member_data = packctx->writer->data + member_pos;
		if( !packctx->writer->dummy )
			dl_internal_patch_member( dl_ctx, member, member_data, (uintptr_t)packctx->writer->data, subdata_pos - member_size );
	}
}

static void dl_txt_pack_member( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx, size_t instance_pos, const dl_member_desc* member )
{
	size_t member_pos = instance_pos + member->offset[DL_PTR_SIZE_HOST];
	dl_binary_writer_seek_set( packctx->writer, member_pos );
	switch( member->AtomType() )
	{
		case DL_TYPE_ATOM_POD:
		{
			// write
			switch( member->StorageType() )
			{
				case DL_TYPE_STORAGE_INT8:   dl_txt_pack_eat_and_write_int8( dl_ctx, packctx );   break;
				case DL_TYPE_STORAGE_INT16:  dl_txt_pack_eat_and_write_int16( dl_ctx, packctx );  break;
				case DL_TYPE_STORAGE_INT32:  dl_txt_pack_eat_and_write_int32( dl_ctx, packctx );  break;
				case DL_TYPE_STORAGE_INT64:  dl_txt_pack_eat_and_write_int64( dl_ctx, packctx );  break;
				case DL_TYPE_STORAGE_UINT8:  dl_txt_pack_eat_and_write_uint8( dl_ctx, packctx );  break;
				case DL_TYPE_STORAGE_UINT16: dl_txt_pack_eat_and_write_uint16( dl_ctx, packctx ); break;
				case DL_TYPE_STORAGE_UINT32: dl_txt_pack_eat_and_write_uint32( dl_ctx, packctx ); break;
				case DL_TYPE_STORAGE_UINT64: dl_txt_pack_eat_and_write_uint64( dl_ctx, packctx ); break;
				case DL_TYPE_STORAGE_FP32:   dl_txt_pack_eat_and_write_fp32( dl_ctx, packctx );   break;
				case DL_TYPE_STORAGE_FP64:   dl_txt_pack_eat_and_write_fp64( dl_ctx, packctx );   break;
				case DL_TYPE_STORAGE_STR:    dl_txt_pack_eat_and_write_string( dl_ctx, packctx ); break;
				case DL_TYPE_STORAGE_PTR:    dl_txt_pack_eat_and_write_ptr( dl_ctx, packctx, dl_internal_find_type( dl_ctx, member->type_id ), member_pos ); break;
				case DL_TYPE_STORAGE_STRUCT: dl_txt_pack_eat_and_write_struct( dl_ctx, packctx, dl_internal_find_type( dl_ctx, member->type_id ) ); break;
				case DL_TYPE_STORAGE_ENUM_INT8:
				case DL_TYPE_STORAGE_ENUM_INT16:
				case DL_TYPE_STORAGE_ENUM_INT32:
				case DL_TYPE_STORAGE_ENUM_INT64:
				case DL_TYPE_STORAGE_ENUM_UINT8:
				case DL_TYPE_STORAGE_ENUM_UINT16:
				case DL_TYPE_STORAGE_ENUM_UINT32:
				case DL_TYPE_STORAGE_ENUM_UINT64:
				{
					dl_txt_eat_white( &packctx->read_ctx );
					const dl_enum_desc* edesc = dl_internal_find_enum( dl_ctx, member->type_id );
					if( edesc == 0x0 )
						dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TYPE_NOT_FOUND, "couldn't find enum-type of <type_name_here>.%s", dl_internal_member_name( dl_ctx, member ) );
					dl_txt_pack_eat_and_write_enum( dl_ctx, packctx, edesc );
				}
				break;
				default:
					DL_ASSERT(false);
			}
		}
		break;
		case DL_TYPE_ATOM_ARRAY:
		{
			dl_txt_eat_char( dl_ctx, &packctx->read_ctx, '[' );
			uint32_t array_length = dl_txt_pack_find_array_length( dl_ctx, packctx, member );
			if( array_length == 0 )
			{
				dl_binary_writer_write_pint( packctx->writer, (size_t)-1 );
				dl_binary_writer_write_uint32( packctx->writer, 0 );
			}
			else
			{
				size_t element_size, element_align;
				dl_txt_pack_array_item_size_align( dl_ctx, member, &element_size, &element_align );
				size_t array_pos = dl_binary_writer_needed_size( packctx->writer );
				array_pos = dl_internal_align_up( array_pos, element_align );

				dl_binary_writer_write_pint( packctx->writer, array_pos );
				dl_binary_writer_write_uint32( packctx->writer, array_length );
				dl_binary_writer_seek_end( packctx->writer );
				dl_binary_writer_align( packctx->writer, element_align );
				dl_binary_writer_reserve( packctx->writer, array_length * element_size );
				dl_txt_pack_eat_and_write_array( dl_ctx, packctx, member, array_length );
			}
			dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ']' );
		}
		break;
		case DL_TYPE_ATOM_INLINE_ARRAY:
		{
			dl_txt_eat_char( dl_ctx, &packctx->read_ctx, '[' );
			uint32_t array_length = dl_txt_pack_find_array_length( dl_ctx, packctx, member );
			if(array_length > member->inline_array_cnt())
			{
				dl_txt_read_failed( dl_ctx,
									&packctx->read_ctx,
									DL_ERROR_MALFORMED_DATA,
									"to many elements in inline array, %u > %u", array_length, member->inline_array_cnt() );
			}

			if(array_length > 0)
				dl_txt_pack_eat_and_write_array( dl_ctx, packctx, member, array_length );

			switch(member->StorageType())
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					const dl_type_desc* sub_type = dl_internal_find_type(dl_ctx, member->type_id);

					// fill missing elements with defaults!
					size_t current_member_array_position = member_pos + sub_type->size[DL_PTR_SIZE_HOST] * array_length;
					for(uint32_t i = array_length; i < member->inline_array_cnt(); ++i)
					{
						// TODO: this seek/set dance will only be needed if type has subptrs, optimize by making different code-paths?
						dl_binary_writer_seek_set( packctx->writer, current_member_array_position);

						// TODO: replace with flag DL_FULL_TYPE_DEFAULT saying that all members has default-values.
						for(uint32_t sub_member_i = 0; sub_member_i < sub_type->member_count; ++sub_member_i)
						{
							const dl_member_desc* sub_member = dl_get_type_member(dl_ctx, sub_type, sub_member_i);
							dl_txt_pack_write_default_value(dl_ctx, packctx, sub_member, current_member_array_position + sub_member->offset[DL_PTR_SIZE_HOST]);
						}
							current_member_array_position += sub_type->size[DL_PTR_SIZE_HOST];
					}
					break;
				}
				default:
				{
					// default type to zero!
					uint32_t missing_elements = member->inline_array_cnt() - array_length;
					if(missing_elements > 0)
						dl_binary_writer_write_zero(packctx->writer, missing_elements * dl_pod_size(member->StorageType()));
				}
			}

			dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ']' );
		}
		break;
		case DL_TYPE_ATOM_BITFIELD:
		{
			uint64_t bf_value = dl_txt_pack_eat_strtoull(dl_ctx, &packctx->read_ctx, UINT64_MAX, "uint64");
			uint32_t bf_bits = member->bitfield_bits();
			uint32_t bf_offset = member->bitfield_offset();

			uint64_t max_val = (uint64_t(1) << bf_bits) - uint64_t(1);
			if( bf_value > max_val )
				dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "bitfield value to big, %u > %u", (unsigned int)bf_value, (unsigned int)max_val );

			uint64_t old_val = 0;

			switch( member->StorageType() )
			{
				case DL_TYPE_STORAGE_UINT8:  old_val = (uint64_t)dl_binary_writer_read_uint8 ( packctx->writer ); break;
				case DL_TYPE_STORAGE_UINT16: old_val = (uint64_t)dl_binary_writer_read_uint16( packctx->writer ); break;
				case DL_TYPE_STORAGE_UINT32: old_val = (uint64_t)dl_binary_writer_read_uint32( packctx->writer ); break;
				case DL_TYPE_STORAGE_UINT64: old_val = (uint64_t)dl_binary_writer_read_uint64( packctx->writer ); break;
				default:
					DL_ASSERT( false && "This should not happen!" );
					break;
			}

			uint64_t to_store = DL_INSERT_BITS( old_val, bf_value, dl_bf_offset( DL_ENDIAN_HOST, sizeof(uint8_t), bf_offset, bf_bits ), bf_bits );

			switch( member->StorageType() )
			{
				case DL_TYPE_STORAGE_UINT8:  dl_binary_writer_write_uint8 ( packctx->writer,  (uint8_t)to_store ); break;
				case DL_TYPE_STORAGE_UINT16: dl_binary_writer_write_uint16( packctx->writer, (uint16_t)to_store ); break;
				case DL_TYPE_STORAGE_UINT32: dl_binary_writer_write_uint32( packctx->writer, (uint32_t)to_store ); break;
				case DL_TYPE_STORAGE_UINT64: dl_binary_writer_write_uint64( packctx->writer, (uint64_t)to_store ); break;
				default:
					DL_ASSERT( false && "This should not happen!" );
					break;
			}
		}
		break;
		default:
			DL_ASSERT(false);
	}
}

static void dl_txt_pack_eat_and_write_array_struct( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx, const dl_type_desc* type )
{
	dl_txt_eat_char( dl_ctx, &packctx->read_ctx, '[' );

	// ... reserve space for the type ...
	size_t instance_pos = dl_binary_writer_tell( packctx->writer ); // duplicated in dl_txt_pack_eat_and_write_struct() =/
	dl_binary_writer_reserve( packctx->writer, type->size[DL_PTR_SIZE_HOST] );

	// ... pack all members in order ...
	for( uint32_t member_index = 0; member_index < type->member_count; ++member_index )
	{
		dl_txt_pack_member( dl_ctx, packctx, instance_pos, dl_get_type_member( dl_ctx, type, member_index ) );

		dl_txt_eat_white( &packctx->read_ctx );

		if( member_index < type->member_count - 1 )
		{
			if( *packctx->read_ctx.iter != ',' )
				dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TXT_MISSING_MEMBER,
									"type '%s' was specified as array, only %u of %u members set!",
									dl_internal_type_name( dl_ctx, type ),
									member_index + 1,
									type->member_count );
			else
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ',' );
		}
	}

	dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ']' );
}

static const dl_txt_read_substr dl_txt_eat_object_key( dl_txt_read_ctx* readctx )
{
	dl_txt_read_substr res = {0x0, 0};
	switch(*readctx->iter)
	{
		case '"':
		case '\'':
			return dl_txt_eat_string( readctx );
		default:
		{
			const char* key_start = readctx->iter;
			const char* key_end = key_start;
			while(isalnum(*key_end) || *key_end == '_')
				++key_end;

			if(key_start != key_end)
			{
				res.str = key_start;
				res.len = (int)(key_end - key_start);
				readctx->iter = res.str + res.len;
				return res;
			}
		}
		break;
	}
	return res;
}

static void dl_txt_pack_eat_and_write_struct( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx, const dl_type_desc* type )
{
	uint64_t members_set = 0;
	bool     union_member_set = false;
	uint32_t member_name_hash = 0;

	dl_txt_eat_white( &packctx->read_ctx );

	if( *packctx->read_ctx.iter == '[' )
	{
		dl_txt_pack_eat_and_write_array_struct( dl_ctx, packctx, type );
		return;
	}

	// ... find open {
	dl_txt_eat_char( dl_ctx, &packctx->read_ctx, '{' );

	// ... reserve space for the type ...
	size_t instance_pos = dl_binary_writer_tell( packctx->writer );
	dl_binary_writer_reserve( packctx->writer, type->size[DL_PTR_SIZE_HOST] );

	while( true )
	{
		// ... read all members ...
		dl_txt_eat_white( &packctx->read_ctx );
		if( *packctx->read_ctx.iter == ',' ) ++packctx->read_ctx.iter;
		// We allow traailing commas, so consume whitespaces to see if we have a struct termination.
		dl_txt_eat_white( &packctx->read_ctx );
		if( *packctx->read_ctx.iter == '}' ) break;

		dl_txt_eat_white( &packctx->read_ctx );
		dl_txt_read_substr member_name = dl_txt_eat_object_key( &packctx->read_ctx );
		if( member_name.str == 0x0 )
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "expected map-key containing member name." );

		if( member_name.str[0] == '_' && member_name.str[1] == '_' )
		{
			// ... members with __ are reserved for dl, check if __subdata map, otherwise warn ...
			if( strncmp( "__subdata", member_name.str, 9 ) == 0 )
			{
				dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ':' );

				if( packctx->subdata_pos )
					dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "\"__subdata\" set twice!" );

				packctx->subdata_pos = packctx->read_ctx.iter;
				packctx->read_ctx.iter = dl_txt_skip_map( packctx->read_ctx.iter, packctx->read_ctx.end );
				continue;
			}
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TXT_INVALID_MEMBER, "type %s has no member named %.*s", dl_internal_type_name( dl_ctx, type ), member_name.len, member_name.str );
		}

		if( type->flags & DL_TYPE_FLAG_IS_UNION )
		{
			if( union_member_set )
			{
				dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TXT_MULTIPLE_MEMBERS_IN_UNION_SET, "multiple members of union-type was set! %s.%.*s", dl_internal_type_name( dl_ctx, type ), member_name.len, member_name.str );
			}
			union_member_set = true;
		}

		member_name_hash = dl_internal_hash_buffer( (const uint8_t*)member_name.str, (size_t)member_name.len);
		unsigned int member_id = dl_internal_find_member( dl_ctx, type, member_name_hash );
		if( member_id > type->member_count )
		{
			dl_txt_pack_validate_c_symbol_key(dl_ctx, packctx, member_name);
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TXT_INVALID_MEMBER, "type '%s' has no member named '%.*s'", dl_internal_type_name( dl_ctx, type ), member_name.len, member_name.str );
		}

		uint64_t member_bit = ( 1ULL << member_id );
		if( member_bit & members_set )
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TXT_MEMBER_SET_TWICE, "member '%s.%.*s' is set twice", dl_internal_type_name( dl_ctx, type ), member_name.len, member_name.str );
		members_set |= member_bit;

		dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ':' );

		// ... read member ...
		dl_txt_pack_member( dl_ctx, packctx, instance_pos, dl_get_type_member( dl_ctx, type, member_id ) );
	}

	// ... find close }
	dl_txt_eat_char( dl_ctx, &packctx->read_ctx, '}' );

	// ... finalize members ...
	if( type->flags & DL_TYPE_FLAG_IS_UNION )
	{
		size_t type_offset = dl_internal_union_type_offset( dl_ctx, type, DL_PTR_SIZE_HOST );
		dl_binary_writer_seek_set( packctx->writer, instance_pos + type_offset );
		dl_binary_writer_write_uint32( packctx->writer, member_name_hash );
	}
	else
	{
		for( uint32_t i = 0; i < type->member_count; ++i )
		{
			if( members_set & ( 1ULL << i ) )
				continue;

			const dl_member_desc* member = dl_get_type_member( dl_ctx, type, i );
			if( member->default_value_offset == UINT32_MAX )
				dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TXT_MISSING_MEMBER, "member %s.%s is not set and has no default value", dl_internal_type_name( dl_ctx, type ), dl_internal_member_name( dl_ctx, member ) );

			size_t   member_pos = instance_pos + member->offset[DL_PTR_SIZE_HOST];
			dl_txt_pack_write_default_value(dl_ctx, packctx, member, member_pos);
		}
	}
}

static dl_error_t dl_txt_pack_finalize_subdata( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
	if( packctx->subdata_count == 0 )
		return DL_ERROR_OK;
	if( packctx->subdata_pos == 0x0 )
		dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TXT_MISSING_SECTION, "instance has pointers but no \"__subdata\"-member" );

	packctx->read_ctx.iter = packctx->subdata_pos;

	struct
	{
		dl_txt_read_substr name;
		size_t pos;
	} subinstances[256];
	subinstances[0].name.str = "__root";
	subinstances[0].name.len = 6;
	subinstances[0].pos = 0;
	size_t subinstances_count = 1;

	dl_txt_eat_char( dl_ctx, &packctx->read_ctx, '{' );

	while( true )
	{
		dl_txt_eat_white( &packctx->read_ctx );
		if( *packctx->read_ctx.iter != '"' )
			break;

		dl_txt_read_substr subdata_name = dl_txt_eat_string( &packctx->read_ctx );
		if( subdata_name.str == 0x0 )
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "expected map-key containing subdata instance-name." );

		dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ':' );

		int subdata_item = -1;
		for( int i = 0; i < packctx->subdata_count; ++i )
		{
			if( packctx->subdata[i].name.len != subdata_name.len )
				continue;

			if( strncmp( packctx->subdata[i].name.str, subdata_name.str, (size_t)subdata_name.len ) == 0 )
			{
				subdata_item = i;
				break;
			}
		}

		if( subdata_item < 0 )
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "non-used subdata." );
		const dl_type_desc* type = packctx->subdata[subdata_item].type;

		dl_binary_writer_seek_end( packctx->writer );
		dl_binary_writer_align( packctx->writer, type->alignment[DL_PTR_SIZE_HOST] );
		size_t inst_pos = dl_binary_writer_tell( packctx->writer );

		dl_txt_pack_eat_and_write_struct( dl_ctx, packctx, type );

		if( subinstances_count >= DL_ARRAY_LENGTH(subinstances) )
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "to many subdatas." );
		subinstances[subinstances_count].name = subdata_name;
		subinstances[subinstances_count].pos = inst_pos;
		++subinstances_count;

		dl_txt_eat_white( &packctx->read_ctx );
		if( packctx->read_ctx.iter[0] == ',' )
			++packctx->read_ctx.iter;
		else if( packctx->read_ctx.iter[0] != '}' )
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "expected ',' or '}'." );
	}

	dl_txt_eat_char( dl_ctx, &packctx->read_ctx, '}' );

	for( int i = 0; i < packctx->subdata_count; ++i )
	{
		bool found = false;
		for( size_t j = 0; j < subinstances_count; ++j )
		{
			if( packctx->subdata[i].name.len != subinstances[j].name.len )
				continue;

			if( strncmp( packctx->subdata[i].name.str, subinstances[j].name.str, (size_t)subinstances[j].name.len ) == 0 )
			{
				found = true;
				dl_binary_writer_seek_set( packctx->writer, packctx->subdata[i].patch_pos );
				dl_binary_writer_write_ptr( packctx->writer, subinstances[j].pos );
			}
		}

		if( !found )
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "referenced subdata \"%.*s\"", packctx->subdata[i].name.len, packctx->subdata[i].name.str );
	}

	return DL_ERROR_OK;
}

static const dl_type_desc* dl_txt_pack_inner( dl_ctx_t dl_ctx, dl_txt_pack_ctx* packctx )
{
#if defined(_MSC_VER )
#pragma warning(push)
#pragma warning(disable:4611)
#endif
	if( setjmp( packctx->read_ctx.jumpbuf ) == 0 )
#if defined(_MSC_VER )
#pragma warning(pop)
#endif
	{
		// ... find open { for top map
		dl_txt_eat_char( dl_ctx, &packctx->read_ctx, '{' );

		// ... find first and only key, the type name of the type to pack ...
		dl_txt_eat_white( &packctx->read_ctx );
		dl_txt_read_substr root_type_name = dl_txt_eat_object_key( &packctx->read_ctx );
		if( root_type_name.str == 0x0 )
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_MALFORMED_DATA, "expected map-key with root type name" );

		char type_name[1024] = {0}; // TODO: make a dl_internal_find_type_by_name() that take string name.
		strncpy( type_name, root_type_name.str, (size_t)root_type_name.len );
		const dl_type_desc* root_type = dl_internal_find_type_by_name( dl_ctx, type_name );
		if( root_type == 0x0 )
		{
			dl_txt_pack_validate_c_symbol_key(dl_ctx, packctx, root_type_name);
			dl_txt_read_failed( dl_ctx, &packctx->read_ctx, DL_ERROR_TYPE_NOT_FOUND, "root type was set as \"%s\", but no such type was loaded.", type_name );
		}

		dl_txt_eat_char( dl_ctx, &packctx->read_ctx, ':' );
		dl_txt_pack_eat_and_write_struct( dl_ctx, packctx, root_type );
		dl_txt_eat_char( dl_ctx, &packctx->read_ctx, '}' );

		dl_txt_pack_finalize_subdata( dl_ctx, packctx );
		return root_type;
	}
	return 0x0;
}

dl_error_t dl_txt_pack( dl_ctx_t dl_ctx, const char* txt_instance, unsigned char* out_buffer, size_t out_buffer_size, size_t* produced_bytes )
{
	dl_binary_writer writer;
	dl_binary_writer_init( &writer,
						   out_buffer + sizeof(dl_data_header),
						   out_buffer_size - sizeof(dl_data_header),
						   out_buffer_size == 0,
						   DL_ENDIAN_HOST,
						   DL_ENDIAN_HOST,
						   DL_PTR_SIZE_HOST );
	dl_txt_pack_ctx packctx;
	packctx.writer  = &writer;
	packctx.read_ctx.start = txt_instance;
	packctx.read_ctx.end   = txt_instance + strlen(txt_instance); // TODO: pass to function!
	packctx.read_ctx.iter  = txt_instance;
	packctx.subdata_pos = 0x0;
	packctx.subdata_count = 0;
	packctx.read_ctx.err = DL_ERROR_OK;

	const dl_type_desc* root_type = dl_txt_pack_inner( dl_ctx, &packctx );
	if( packctx.read_ctx.err == DL_ERROR_OK )
	{
		// write header
		if( out_buffer_size > 0 )
		{
			dl_data_header header;
			memset(&header, 0x0, sizeof(dl_data_header));
			header.id                 = DL_INSTANCE_ID;
			header.version            = DL_INSTANCE_VERSION;
			header.root_instance_type = dl_internal_typeid_of( dl_ctx, root_type );
			header.instance_size      = (uint32_t)dl_binary_writer_needed_size( &writer );
			header.is_64_bit_ptr      = sizeof(void*) == 8 ? 1 : 0;
			memcpy( out_buffer, &header, sizeof(dl_data_header) );
		}

		if( produced_bytes )
			*produced_bytes = (unsigned int)dl_binary_writer_needed_size( &writer ) + sizeof(dl_data_header);
	}
	else
	{
		dl_report_error_location( dl_ctx, packctx.read_ctx.start, packctx.read_ctx.end, packctx.read_ctx.iter );
	}
	return packctx.read_ctx.err;
}

dl_error_t dl_txt_pack_calc_size( dl_ctx_t dl_ctx, const char* txt_instance, size_t* out_instance_size )
{
	return dl_txt_pack( dl_ctx, txt_instance, 0x0, 0, out_instance_size );
}
