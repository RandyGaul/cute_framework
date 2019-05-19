#include <dl/dl_typelib.h>
#include <dl/dl_reflect.h>

#include "dl_binary_writer.h"

#include <stdlib.h>
#include <ctype.h>

#if defined( __GNUC__ )
static void dl_binary_writer_write_string_fmt( dl_binary_writer* writer, const char* fmt, ... ) __attribute__((format( printf, 2, 3 )));
#endif

static void dl_binary_writer_write_string_fmt( dl_binary_writer* writer, const char* fmt, ... )
{
	char buffer[4096];
	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	size_t written = (size_t)vsnprintf( buffer, DL_ARRAY_LENGTH( buffer ), fmt, arg_ptr );
	va_end(arg_ptr);

	dl_binary_writer_write( writer, buffer, written );
}

static void dl_context_write_c_header_begin( dl_binary_writer* writer, const char* module_name_uppercase )
{
	dl_binary_writer_write_string_fmt( writer, "/* Auto generated header for dl type library */\n" );
	dl_binary_writer_write_string_fmt( writer, "#ifndef __DL_AUTOGEN_HEADER_%s_INCLUDED\n", module_name_uppercase );
	dl_binary_writer_write_string_fmt( writer, "#define __DL_AUTOGEN_HEADER_%s_INCLUDED\n\n", module_name_uppercase );
	dl_binary_writer_write_string_fmt( writer,
									   "#include <stdint.h>\n\n"
									   "#include <stddef.h> // for size_t and offsetof\n\n"
									   "#ifndef __DL_AUTOGEN_HEADER_DL_ALIGN_DEFINED\n"
									   "#define __DL_AUTOGEN_HEADER_DL_ALIGN_DEFINED\n"
									   "\n"
                                       "#  if !defined(__cplusplus)\n"
                                       "#    define DL_SIZED_ENUM_SUPPORTED 0\n"
                                       "#  else\n"
                                       "#    if defined(_MSC_VER)\n"
                                       "#      if _MSC_VER < 1900\n"
                                       "#        define DL_SIZED_ENUM_SUPPORTED 0\n"
                                       "#      else\n"
                                       "#        define DL_SIZED_ENUM_SUPPORTED 0\n"
                                       "#      endif\n"
                                       "#    else\n"
                                       "       // this would need more work but it is good enough for now\n"
                                       "#      define DL_SIZED_ENUM_SUPPORTED 1\n"
                                       "#    endif\n"
                                       "#  endif\n"
                                       "\n"
									   "   /// ... DL_C_STRUCT ...\n"
									   "   /// The macro DL_C_STRUCT is used on external types to add struct in c\n"
									   "   /// where it is needed. In c++ this is skipped as it is not needed and\n"
									   "   /// could be something else such as a class or typedef.\n"
									   "#  if defined(__cplusplus)\n"
									   "#    define DL_C_STRUCT\n"
									   "#  else\n"
									   "#    define DL_C_STRUCT struct\n"
									   "#  endif\n"
                                       "\n"
									   "   /// ... DL_ALIGN() ...\n"
									   "#  if defined(_MSC_VER)\n"
									   "#    define DL_ALIGN(x) __declspec(align(x))\n"
									   "#  elif defined(__GNUC__)\n"
									   "#    define DL_ALIGN(x) __attribute__((aligned(x)))\n"
									   "#  else\n"
									   "#    error \"Unsupported compiler\"\n"
									   "#  endif\n"
									   "\n"
									   "   /// ... DL_ALIGNOF() ...\n"
									   "#  if defined(__clang__)\n"
									   "#    if __has_feature(cxx_alignof)\n"
									   "#      define DL_ALIGNOF(type) alignof(type)\n"
									   "#    elif __has_feature(c_alignof)\n"
									   "#      define DL_ALIGNOF(type) _Alignof(type)\n"
									   "#    endif\n"
									   "#  endif\n"
									   "#  if defined(_MSC_VER )\n"
									   "#    define DL_ALIGNOF(type) __alignof(type)\n"
									   "#  endif\n"
									   "\n"
									   "#  ifndef DL_ALIGNOF\n"
									   "#    define DL_ALIGNOF(type) ((sizeof(type) > 1)? offsetof(struct { char c; type x; }, x) : 1)\n"
									   //"#    define DL_ALIGNOF(type) alignof(type)\n"
									   "#  endif\n"
									   "\n"
									   "   // ... DL_STATIC_ASSERT() ...\n"
									   "   // ... clang ...\n"
									   "#  if defined( __clang__ )\n"
									   "#    if defined( __cplusplus ) && __has_feature(cxx_static_assert)\n"
									   "#      define DL_STATIC_ASSERT( cond, msg ) static_assert( cond, msg )\n"
									   "#    elif __has_feature(c_static_assert)\n"
									   "#      define DL_STATIC_ASSERT( cond, msg ) _Static_assert( cond, msg )\n"
									   "#    endif\n"
									   "   // ... msvc ...\n"
									   "#  elif defined( _MSC_VER ) && ( defined(_MSC_VER) && (_MSC_VER >= 1600) )\n"
									   "#    define DL_STATIC_ASSERT( cond, msg ) static_assert( cond, msg )\n"
									   "   // ... gcc ...\n"
									   "#  elif defined( __cplusplus )\n"
									   "#    if __cplusplus >= 201103L || ( defined(_MSC_VER) && (_MSC_VER >= 1600) )\n"
									   "#      define DL_STATIC_ASSERT( cond, msg ) static_assert( cond, msg )\n"
									   "#    endif\n"
									   "#  elif defined( __STDC__ )\n"
									   "#    if defined( __STDC_VERSION__ )\n"
									   "#      if __STDC_VERSION__ >= 201112L\n"
									   "#        define DL_STATIC_ASSERT( cond, msg ) _Static_assert( cond, msg )\n"
									   "#      else\n"
									   "#        define DL_STATIC_ASSERT( cond, msg ) _Static_assert( cond, msg )\n"
									   "#      endif\n"
									   "#    endif\n"
									   "#  endif\n"
									   "#  if !defined(DL_STATIC_ASSERT)\n"
									   "#    define DL_STATIC_ASSERT(x,y) // default to non-implemented.\n"
									   "#  endif\n"
									   "#endif // __DL_AUTOGEN_HEADER_DL_ALIGN_DEFINED\n\n" );
}

static void dl_context_write_c_header_end( dl_binary_writer* writer, const char* module_name_uppercase )
{
	dl_binary_writer_write_string_fmt( writer, "#endif // __DL_AUTOGEN_HEADER_%s_INCLUDED\n\n", module_name_uppercase );
}

static void dl_context_write_c_header_typeids( dl_binary_writer* writer, dl_ctx_t ctx )
{
	dl_binary_writer_write_string_fmt( writer, "//----------------------------------------------\n"
											   "//                   TYPEID:s                   \n"
											   "//----------------------------------------------\n\n" );

	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	if(ctx_info.num_enums != 0)
	{
		dl_enum_info_t* enum_infos = (dl_enum_info_t*)malloc( ctx_info.num_enums * sizeof( dl_enum_info_t ) );
		dl_reflect_loaded_enums(ctx, enum_infos, ctx_info.num_enums);

		dl_binary_writer_write_string_fmt( writer, "// ENUMS\n");

		for( unsigned int enum_index = 0; enum_index < ctx_info.num_enums; ++enum_index )
			dl_binary_writer_write_string_fmt( writer, "#define %s_TYPE_ID (0x%08X)\n", enum_infos[enum_index].name, enum_infos[enum_index].tid );
		free(enum_infos);
	}

	if(ctx_info.num_types != 0)
	{
		dl_type_info_t* type_info = (dl_type_info_t*)malloc( ctx_info.num_types * sizeof( dl_type_info_t ) );
		dl_reflect_loaded_types( ctx, type_info, ctx_info.num_types );

		if(ctx_info.num_enums != 0)
			dl_binary_writer_write_string_fmt( writer, "\n");
		dl_binary_writer_write_string_fmt( writer, "// TYPES\n");

		for( unsigned int type_index = 0; type_index < ctx_info.num_types; ++type_index )
			dl_binary_writer_write_string_fmt( writer, "#define %s_TYPE_ID (0x%08X)\n", type_info[type_index].name, type_info[type_index].tid );

		free(type_info);
	}

	dl_binary_writer_write_string_fmt( writer, "\n\n");
}

static void dl_context_write_c_header_enum_value(dl_binary_writer* writer, dl_type_storage_t storage, const dl_enum_value_info_t* value)
{
	switch(storage)
	{
		case DL_TYPE_STORAGE_ENUM_INT8:   if(value->value.i8  == INT8_MIN)   { dl_binary_writer_write_string( writer, "INT8_MIN",    8 ); return; }
										  if(value->value.i8  == INT8_MAX)   { dl_binary_writer_write_string( writer, "INT8_MAX",    8 ); return; }
		break;
		case DL_TYPE_STORAGE_ENUM_INT16:  if(value->value.i16 == INT16_MIN)  { dl_binary_writer_write_string( writer, "INT16_MIN",   9 ); return; }
										  if(value->value.i16 == INT16_MAX)  { dl_binary_writer_write_string( writer, "INT16_MAX",   9 ); return; }
		break;
		case DL_TYPE_STORAGE_ENUM_INT32:  if(value->value.i32 == INT32_MIN)  { dl_binary_writer_write_string( writer, "INT32_MIN",   9 ); return; }
										  if(value->value.i32 == INT32_MAX)  { dl_binary_writer_write_string( writer, "INT32_MAX",   9 ); return; }
		break;
		case DL_TYPE_STORAGE_ENUM_INT64:  if(value->value.i64 == INT64_MIN)  { dl_binary_writer_write_string( writer, "INT64_MIN",   9 ); return; }
										  if(value->value.i64 == INT64_MAX)  { dl_binary_writer_write_string( writer, "INT64_MAX",   9 ); return; }
		break;
		case DL_TYPE_STORAGE_ENUM_UINT8:  if(value->value.u8  == UINT8_MAX)  { dl_binary_writer_write_string( writer, "UINT8_MAX",   9 ); return; }
		break;
		case DL_TYPE_STORAGE_ENUM_UINT16: if(value->value.u16 == UINT16_MAX) { dl_binary_writer_write_string( writer, "UINT16_MAX", 10 ); return; }
		break;
		case DL_TYPE_STORAGE_ENUM_UINT32: if(value->value.u32 == UINT32_MAX) { dl_binary_writer_write_string( writer, "UINT32_MAX", 10 ); return; }
		break;
		case DL_TYPE_STORAGE_ENUM_UINT64: if(value->value.u64 == UINT64_MAX) { dl_binary_writer_write_string( writer, "UINT64_MAX", 10 ); return; }
		break;
		default:
			DL_ASSERT(false);
	}

	switch(storage)
	{
		case DL_TYPE_STORAGE_ENUM_INT8:
		case DL_TYPE_STORAGE_ENUM_INT16:
		case DL_TYPE_STORAGE_ENUM_INT32:
		case DL_TYPE_STORAGE_ENUM_INT64:
			dl_binary_writer_write_string_fmt( writer, DL_INT64_FMT_STR, value->value.i64 );
			break;
		case DL_TYPE_STORAGE_ENUM_UINT8:
		case DL_TYPE_STORAGE_ENUM_UINT16:
		case DL_TYPE_STORAGE_ENUM_UINT32:
		case DL_TYPE_STORAGE_ENUM_UINT64:
			dl_binary_writer_write_string_fmt( writer, DL_UINT64_FMT_STR, value->value.u64 );
			break;
		default:
			DL_ASSERT(false);
	}
}

static const char* dl_context_enum_storage_decl(dl_type_storage_t storage)
{
	switch(storage)
	{
		case DL_TYPE_STORAGE_ENUM_INT8:   return " : int8_t";
		case DL_TYPE_STORAGE_ENUM_INT16:  return " : int16_t";
		case DL_TYPE_STORAGE_ENUM_INT32:  return " : int32_t";
		case DL_TYPE_STORAGE_ENUM_INT64:  return " : int64_t";
		case DL_TYPE_STORAGE_ENUM_UINT8:  return " : uint8_t";
		case DL_TYPE_STORAGE_ENUM_UINT16: return " : uint16_t";
		case DL_TYPE_STORAGE_ENUM_UINT32: return ""; // no decl on 32-bit enums as that is default-size.
		case DL_TYPE_STORAGE_ENUM_UINT64: return" : uint64_t"; break;
		default:
			DL_ASSERT(false);
			return "";
	}
}

static void dl_context_write_c_header_enum( dl_binary_writer* writer, dl_ctx_t ctx, dl_enum_info_t* e_info )
{
	if( e_info->is_extern )
		return;

	if( e_info->comment )
		dl_binary_writer_write_string_fmt( writer, "//%s\n",  e_info->comment );

	dl_binary_writer_write_string_fmt( writer, "typedef enum %s%s\n{\n", e_info->name, dl_context_enum_storage_decl(e_info->storage) );

	dl_enum_value_info_t* values = (dl_enum_value_info_t*)malloc( e_info->value_count * sizeof( dl_enum_value_info_t ) );
	dl_reflect_get_enum_values( ctx, e_info->tid, values, e_info->value_count );

	for( unsigned int j = 0; j < e_info->value_count; ++j )
	{
		if(j > 0)
			dl_binary_writer_write_string( writer, ",\n", 2 );
		dl_binary_writer_write_string_fmt(writer, "    %s = ", values[j].name);
		dl_context_write_c_header_enum_value( writer, e_info->storage, &values[j]);
	}

	dl_binary_writer_write_string_fmt( writer, "\n} %s;\n\n", e_info->name );
	free( values );
}

/**
 * Writes a fallback for sized enums based on typedef and constants.
 */
static void dl_context_write_c_header_enum_fallback( dl_binary_writer* writer, dl_ctx_t ctx, dl_enum_info_t* e_info )
{
	if( e_info->is_extern )
		return;

    switch(e_info->storage)
	{
		case DL_TYPE_STORAGE_ENUM_INT8:   dl_binary_writer_write_string_fmt( writer, "typedef int8_t %s;\n"  , e_info->name); break;
		case DL_TYPE_STORAGE_ENUM_INT16:  dl_binary_writer_write_string_fmt( writer, "typedef int16_t %s;\n" , e_info->name); break;
		case DL_TYPE_STORAGE_ENUM_INT32:  dl_binary_writer_write_string_fmt( writer, "typedef int32_t %s;\n" , e_info->name); break;
		case DL_TYPE_STORAGE_ENUM_INT64:  dl_binary_writer_write_string_fmt( writer, "typedef int64_t %s;\n" , e_info->name); break;
		case DL_TYPE_STORAGE_ENUM_UINT8:  dl_binary_writer_write_string_fmt( writer, "typedef uint8_t %s;\n" , e_info->name); break;
		case DL_TYPE_STORAGE_ENUM_UINT16: dl_binary_writer_write_string_fmt( writer, "typedef uint16_t %s;\n", e_info->name); break;
		case DL_TYPE_STORAGE_ENUM_UINT32: dl_binary_writer_write_string_fmt( writer, "typedef uint32_t %s;\n", e_info->name); break;
		case DL_TYPE_STORAGE_ENUM_UINT64: dl_binary_writer_write_string_fmt( writer, "typedef uint64_t %s;\n", e_info->name); break;
		default:
			/*ignore*/
			break;
	}

	dl_enum_value_info_t* values = (dl_enum_value_info_t*)malloc( e_info->value_count * sizeof( dl_enum_value_info_t ) );
	dl_reflect_get_enum_values( ctx, e_info->tid, values, e_info->value_count );

	for( unsigned int j = 0; j < e_info->value_count; ++j )
	{
		dl_binary_writer_write_string_fmt( writer, "static const %s %s = ", e_info->name, values[j].name );
		dl_context_write_c_header_enum_value( writer, e_info->storage, &values[j] );
		dl_binary_writer_write_string_fmt( writer, ";\n");
	}
	free( values );

	dl_binary_writer_write_string_fmt( writer, "\n");
}

static void dl_context_write_c_header_enums( dl_binary_writer* writer, dl_ctx_t ctx )
{
	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	if(ctx_info.num_enums == 0)
		return;

	dl_binary_writer_write_string_fmt( writer, "//----------------------------------------------\n"
											   "//                   ENUMS:s                    \n"
											   "//----------------------------------------------\n\n" );

	dl_enum_info_t* enum_infos = (dl_enum_info_t*)malloc( ctx_info.num_enums * sizeof( dl_enum_info_t ) );
	dl_reflect_loaded_enums(ctx, enum_infos, ctx_info.num_enums);

	unsigned int extern_cnt = 0;

	// generate static checks for extern enums
	for( unsigned int enum_index = 0; enum_index < ctx_info.num_enums; ++enum_index )
	{
		dl_enum_info_t* enum_ = enum_infos + enum_index;
		if(!enum_->is_extern)
			continue;

		extern_cnt++;

		unsigned int enum_size = 0;
		switch(enum_->storage)
		{
			case DL_TYPE_STORAGE_ENUM_INT8:
			case DL_TYPE_STORAGE_ENUM_UINT8:  enum_size = 1; break;
			case DL_TYPE_STORAGE_ENUM_INT16:
			case DL_TYPE_STORAGE_ENUM_UINT16: enum_size = 2; break;
			case DL_TYPE_STORAGE_ENUM_INT32:
			case DL_TYPE_STORAGE_ENUM_UINT32: enum_size = 4; break;
			case DL_TYPE_STORAGE_ENUM_INT64:
			case DL_TYPE_STORAGE_ENUM_UINT64: enum_size = 8; break;
			default:
				/*ignore*/
				break;
		};

		dl_binary_writer_write_string_fmt( writer, "DL_STATIC_ASSERT(sizeof(enum %s) == %u, \"size of external enum %s do not match what was specified in tld.\");\n", enum_->name, enum_size, enum_->name );

		dl_enum_value_info_t* values = (dl_enum_value_info_t*)malloc( enum_->value_count * sizeof( dl_enum_value_info_t ) );
		dl_reflect_get_enum_values( ctx, enum_->tid, values, enum_->value_count );

		for( unsigned int j = 0; j < enum_->value_count; ++j )
		{
			dl_binary_writer_write_string_fmt( writer, "DL_STATIC_ASSERT(%s == ", values[j].name );
			dl_context_write_c_header_enum_value( writer, enum_->storage, &values[j]);
			dl_binary_writer_write_string_fmt( writer, ", \"value of enum-value do not match what was specified in tld.\");\n" );
		}

		free(values);

		dl_binary_writer_write_string_fmt( writer, "\n" );
	}

	if(extern_cnt != ctx_info.num_enums)
	{
		dl_binary_writer_write_string_fmt( writer, "#if DL_SIZED_ENUM_SUPPORTED\n\n" );

		for( unsigned int enum_index = 0; enum_index < ctx_info.num_enums; ++enum_index )
			dl_context_write_c_header_enum( writer, ctx, &enum_infos[enum_index] );

		dl_binary_writer_write_string_fmt( writer, "#else // DL_SIZED_ENUM_SUPPORTED\n\n");

		for( unsigned int enum_index = 0; enum_index < ctx_info.num_enums; ++enum_index )
		{
			if(enum_infos[enum_index].storage == DL_TYPE_STORAGE_ENUM_UINT32)
				dl_context_write_c_header_enum( writer, ctx, &enum_infos[enum_index] );
			else
				dl_context_write_c_header_enum_fallback( writer, ctx, &enum_infos[enum_index] );
		}

		dl_binary_writer_write_string_fmt( writer, "#endif // DL_SIZED_ENUM_SUPPORTED\n\n");
	}

	free( enum_infos );
}

static void dl_context_write_type( dl_ctx_t ctx, dl_type_storage_t storage, dl_typeid_t tid, dl_binary_writer* writer )
{
	switch( storage )
	{
		case DL_TYPE_STORAGE_STRUCT:
		{
			dl_type_info_t sub_type;
			dl_reflect_get_type_info( ctx, tid, &sub_type );
			dl_binary_writer_write_string_fmt(writer, "DL_C_STRUCT %s", sub_type.name);
			return;
		}
		case DL_TYPE_STORAGE_ENUM_INT8:
		case DL_TYPE_STORAGE_ENUM_INT16:
		case DL_TYPE_STORAGE_ENUM_INT32:
		case DL_TYPE_STORAGE_ENUM_UINT8:
		case DL_TYPE_STORAGE_ENUM_UINT16:
		case DL_TYPE_STORAGE_ENUM_UINT32:
		{
			dl_enum_info_t enum_info;
			dl_reflect_get_enum_info( ctx, tid, &enum_info );
			dl_binary_writer_write_string_fmt(writer, "%s", enum_info.name);
			return;
		}
		case DL_TYPE_STORAGE_ENUM_INT64:
		case DL_TYPE_STORAGE_ENUM_UINT64:
		{
			dl_enum_info_t enum_info;
			dl_reflect_get_enum_info( ctx, tid, &enum_info );
			dl_binary_writer_write_string_fmt(writer, "DL_ALIGN(8) %s", enum_info.name);
			return;
		}
		case DL_TYPE_STORAGE_INT8:   dl_binary_writer_write_string_fmt(writer, "int8_t"); return;
		case DL_TYPE_STORAGE_UINT8:  dl_binary_writer_write_string_fmt(writer, "uint8_t"); return;
		case DL_TYPE_STORAGE_INT16:  dl_binary_writer_write_string_fmt(writer, "int16_t"); return;
		case DL_TYPE_STORAGE_UINT16: dl_binary_writer_write_string_fmt(writer, "uint16_t"); return;
		case DL_TYPE_STORAGE_INT32:  dl_binary_writer_write_string_fmt(writer, "int32_t"); return;
		case DL_TYPE_STORAGE_UINT32: dl_binary_writer_write_string_fmt(writer, "uint32_t"); return;
		case DL_TYPE_STORAGE_INT64:  dl_binary_writer_write_string_fmt(writer, "DL_ALIGN(8) int64_t"); return;
		case DL_TYPE_STORAGE_UINT64: dl_binary_writer_write_string_fmt(writer, "DL_ALIGN(8) uint64_t"); return;
		case DL_TYPE_STORAGE_FP32:   dl_binary_writer_write_string_fmt(writer, "float"); return;
		case DL_TYPE_STORAGE_FP64:   dl_binary_writer_write_string_fmt(writer, "DL_ALIGN(8) double"); return;
		case DL_TYPE_STORAGE_STR:    dl_binary_writer_write_string_fmt(writer, "const char*"); return;
		case DL_TYPE_STORAGE_PTR:
		{
			dl_type_info_t sub_type;
			dl_reflect_get_type_info( ctx, tid, &sub_type );
			dl_binary_writer_write_string_fmt(writer, "struct %s*", sub_type.name);
			return;
		}
		default:
			dl_binary_writer_write_string_fmt(writer, "UNKNOWN");
			return;
	}
}

static void dl_context_write_operator_array_access_type( dl_ctx_t ctx, dl_type_storage_t storage, dl_typeid_t tid, dl_binary_writer* writer )
{
	switch( storage )
	{
		case DL_TYPE_STORAGE_INT64:  dl_binary_writer_write_string_fmt(writer, "int64_t");  return;
		case DL_TYPE_STORAGE_UINT64: dl_binary_writer_write_string_fmt(writer, "uint64_t"); return;
		case DL_TYPE_STORAGE_FP64:   dl_binary_writer_write_string_fmt(writer, "double");   return;
		case DL_TYPE_STORAGE_ENUM_INT64:
		case DL_TYPE_STORAGE_ENUM_UINT64:
		{
			dl_enum_info_t enum_info;
			dl_reflect_get_enum_info( ctx, tid, &enum_info );
			dl_binary_writer_write_string_fmt(writer, "%s", enum_info.name);
			return;
		}
		default:
			dl_context_write_type( ctx, storage, tid, writer );
			return;
	}
}

static void dl_context_write_c_header_member( dl_binary_writer* writer, dl_ctx_t ctx, dl_member_info_t* member, bool* last_was_bf )
{
	if( member->comment )
		dl_binary_writer_write_string_fmt( writer, "    //%s\n", member->comment);

	switch( member->atom )
	{
		case DL_TYPE_ATOM_POD:
			switch( member->storage )
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					dl_type_info_t sub_type;
					dl_reflect_get_type_info( ctx, member->type_id, &sub_type );
					dl_binary_writer_write_string_fmt( writer, "    DL_C_STRUCT %s %s;\n", sub_type.name, member->name );
				}
				break;
				case DL_TYPE_STORAGE_PTR:
				{
					dl_type_info_t sub_type;
					dl_reflect_get_type_info( ctx, member->type_id, &sub_type );
					if(member->is_const)
						dl_binary_writer_write_string_fmt( writer, "    const struct %s* %s;\n", sub_type.name, member->name );
					else
						dl_binary_writer_write_string_fmt( writer, "    struct %s* %s;\n", sub_type.name, member->name );
				}
				break;
				case DL_TYPE_STORAGE_ENUM_INT8:
				case DL_TYPE_STORAGE_ENUM_INT16:
				case DL_TYPE_STORAGE_ENUM_INT32:
				case DL_TYPE_STORAGE_ENUM_UINT8:
				case DL_TYPE_STORAGE_ENUM_UINT16:
				case DL_TYPE_STORAGE_ENUM_UINT32:
				{
					dl_enum_info_t sub_type;
					dl_reflect_get_enum_info( ctx, member->type_id, &sub_type );
					dl_binary_writer_write_string_fmt( writer, "    %s %s;\n", sub_type.name, member->name );
				}
				break;
				case DL_TYPE_STORAGE_ENUM_INT64:
				case DL_TYPE_STORAGE_ENUM_UINT64:
				{
					dl_enum_info_t sub_type;
					dl_reflect_get_enum_info( ctx, member->type_id, &sub_type );
					dl_binary_writer_write_string_fmt( writer, "    DL_ALIGN(8) %s %s;\n", sub_type.name, member->name );
				}
				break;
				default:
					dl_binary_writer_write_string_fmt(writer, "    ");
					dl_context_write_type(ctx, member->storage, member->type_id, writer);
					dl_binary_writer_write_string_fmt(writer, " %s; \n", member->name );
					break;
			}

			break;
		case DL_TYPE_ATOM_BITFIELD:
			if( *last_was_bf && member->storage == DL_TYPE_STORAGE_UINT64 )
				dl_binary_writer_write_string_fmt( writer, "    uint64_t %s : %u;\n", member->name, member->bits );
			else
			{
				dl_binary_writer_write_string_fmt(writer, "    ");
				dl_context_write_type(ctx, member->storage, member->type_id, writer);
				dl_binary_writer_write_string_fmt(writer, " %s : %u;\n", member->name, member->bits);
			}
		break;
		case DL_TYPE_ATOM_ARRAY:
		{
			dl_binary_writer_write_string_fmt( writer, "    struct\n    {\n"
													   "        " );
			dl_context_write_operator_array_access_type(ctx, member->storage, member->type_id, writer);
			dl_binary_writer_write_string_fmt( writer, "* data;\n"
													   "        uint32_t count;\n"
													   "    #if defined( __cplusplus )\n"
													   "              ");
			dl_context_write_operator_array_access_type(ctx, member->storage, member->type_id, writer);
			dl_binary_writer_write_string_fmt(writer, "& operator[] ( size_t index ) { return data[index]; }\n");
			switch(member->storage)
			{
				case DL_TYPE_STORAGE_STR:
					dl_binary_writer_write_string_fmt(writer, "        ");
					dl_context_write_operator_array_access_type(ctx, member->storage, member->type_id, writer);
					dl_binary_writer_write_string_fmt(writer, "& operator[] ( size_t index ) const { return data[index]; }\n");
					break;
				case DL_TYPE_STORAGE_PTR:
					dl_binary_writer_write_string_fmt(writer, "        const ");
					dl_context_write_operator_array_access_type(ctx, member->storage, member->type_id, writer);
					dl_binary_writer_write_string_fmt(writer, " operator[] ( size_t index ) const { return data[index]; }\n");
					break;
				default:
					dl_binary_writer_write_string_fmt(writer, "        const ");
					dl_context_write_operator_array_access_type(ctx, member->storage, member->type_id, writer);
					dl_binary_writer_write_string_fmt(writer, "& operator[] ( size_t index ) const { return data[index]; }\n");
					break;
			}

			// ... write begin() and end() to support range-for ...
			{
				const char* type_str = 0x0;
				switch(member->storage)
				{
					case DL_TYPE_STORAGE_INT8:        type_str = "int8_t";   break;
					case DL_TYPE_STORAGE_INT16:       type_str = "int16_t";  break;
					case DL_TYPE_STORAGE_INT32:       type_str = "int32_t";  break;
					case DL_TYPE_STORAGE_INT64:       type_str = "int64_t";  break;
					case DL_TYPE_STORAGE_UINT8:       type_str = "uint8_t";  break;
					case DL_TYPE_STORAGE_UINT16:      type_str = "uint16_t"; break;
					case DL_TYPE_STORAGE_UINT32:      type_str = "uint32_t"; break;
					case DL_TYPE_STORAGE_UINT64:      type_str = "uint64_t"; break;
					case DL_TYPE_STORAGE_FP32:        type_str = "float";  break;
					case DL_TYPE_STORAGE_FP64:        type_str = "double"; break;
					case DL_TYPE_STORAGE_STR:         type_str = "char*"; break;
					case DL_TYPE_STORAGE_ENUM_INT8:
					case DL_TYPE_STORAGE_ENUM_INT16:
					case DL_TYPE_STORAGE_ENUM_INT32:
					case DL_TYPE_STORAGE_ENUM_INT64:
					case DL_TYPE_STORAGE_ENUM_UINT8:
					case DL_TYPE_STORAGE_ENUM_UINT16:
					case DL_TYPE_STORAGE_ENUM_UINT32:
					case DL_TYPE_STORAGE_ENUM_UINT64:
					{
						dl_enum_info_t enum_info;
						dl_reflect_get_enum_info( ctx, member->type_id, &enum_info );
						type_str = enum_info.name;
						break;
					}
					case DL_TYPE_STORAGE_PTR:
					case DL_TYPE_STORAGE_STRUCT:
					{
						dl_type_info_t type_info;
						dl_reflect_get_type_info( ctx, member->type_id, &type_info );
						type_str = type_info.name;
						break;
					}
					default:
						DL_ASSERT(false);
				}

				if(member->storage == DL_TYPE_STORAGE_PTR)
				{
					dl_binary_writer_write_string_fmt(writer, "        const %s* const* begin() const { return &data[0]; }\n",     type_str);
					dl_binary_writer_write_string_fmt(writer, "        const %s* const* end()   const { return &data[count]; }\n", type_str);
				}
				else if(member->storage == DL_TYPE_STORAGE_STR)
				{
					dl_binary_writer_write_string_fmt(writer, "        const %s* begin() const { return &data[0]; }\n",     type_str);
					dl_binary_writer_write_string_fmt(writer, "        const %s* end()   const { return &data[count]; }\n", type_str);
				}
				else
				{
					dl_binary_writer_write_string_fmt(writer, "              %s* begin()       { return &data[0]; }\n",     type_str);
					dl_binary_writer_write_string_fmt(writer, "              %s* end()         { return &data[count]; }\n", type_str);
					dl_binary_writer_write_string_fmt(writer, "        const %s* begin() const { return &data[0]; }\n",     type_str);
					dl_binary_writer_write_string_fmt(writer, "        const %s* end()   const { return &data[count]; }\n", type_str);
				}
			}

			dl_binary_writer_write_string_fmt( writer, "    #endif // defined( __cplusplus )\n"
													   "    } %s;\n", member->name );
		}
		break;
		case DL_TYPE_ATOM_INLINE_ARRAY:
		{
			dl_binary_writer_write_string_fmt(writer, "    ");
			dl_context_write_type(ctx, member->storage, member->type_id, writer);
			dl_binary_writer_write_string_fmt( writer, " %s[%u];\n", member->name, member->array_count );
		}
		break;
		default:
			DL_ASSERT( false );
	}

	*last_was_bf = member->atom == DL_TYPE_ATOM_BITFIELD;
}

static void dl_context_write_c_header_types( dl_binary_writer* writer, dl_ctx_t ctx )
{
	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	if(ctx_info.num_types == 0)
		return;

	dl_binary_writer_write_string_fmt( writer, "//----------------------------------------------\n"
											   "//                   TYPES:s                    \n"
											   "//----------------------------------------------\n\n" );

	dl_type_info_t* type_info = (dl_type_info_t*)malloc( ctx_info.num_types * sizeof( dl_type_info_t ) );
	dl_reflect_loaded_types( ctx, type_info, ctx_info.num_types );

	// ... write checks for extern types ...
	for( unsigned int type_index = 0; type_index < ctx_info.num_types; ++type_index )
	{
		dl_type_info_t* type = &type_info[type_index];
		if( !type->is_extern )
			continue;

		if( type->should_verify )
		{
			dl_binary_writer_write_string_fmt( writer, "// verify that extern type '%s' match the actual type\n", type->name );
			dl_binary_writer_write_string_fmt( writer, "DL_STATIC_ASSERT( sizeof(DL_C_STRUCT %s) == %u, \"size of external type %s do not match what was specified in tld.\" );\n", type->name, type->size, type->name );
			dl_binary_writer_write_string_fmt( writer, "DL_STATIC_ASSERT( DL_ALIGNOF(DL_C_STRUCT %s) == %u, \"alignment of external type %s do not match what was specified in tld.\" );\n", type->name, type->alignment, type->name );
		}
		else
		{
			dl_binary_writer_write_string_fmt( writer, "// '%s' is marked as 'verify : false' so size and align will not be verified\n", type->name );
		}

		dl_member_info_t* members = (dl_member_info_t*)malloc( type->member_count * sizeof( dl_member_info_t ) );
		dl_reflect_get_type_members( ctx, type->tid, members, type->member_count );
		for( unsigned int member_index = 0; member_index < type->member_count; ++member_index )
		{
			dl_member_info_t* member = members + member_index;
			if(member->should_verify)
			{
				dl_binary_writer_write_string_fmt( writer, "DL_STATIC_ASSERT( sizeof(((struct %s*) 0)->%s) == %u, \"sizeof of member %s::%s in external type do not match what was specified in tld.\" );\n",
															type->name,
															member->name,
															member->size,
															type->name,
															member->name );
				dl_binary_writer_write_string_fmt( writer, "DL_STATIC_ASSERT( offsetof(struct %s, %s) == %u, \"offset of member %s::%s in external type do not match what was specified in tld.\" );\n",
															type->name,
															member->name,
															member->offset,
															type->name,
															member->name );
			}
			else
			{
				dl_binary_writer_write_string_fmt( writer, "// %s::%s is marked as 'verify : false' so size and offset will not be verified\n",
															type->name,
															member->name );
			}
		}
		free( members );
	}

	for( unsigned int type_index = 0; type_index < ctx_info.num_types; ++type_index )
	{
		dl_type_info_t* type = &type_info[type_index];

		// if the type is "extern" no header struct should be generated for it.
		if( type->is_extern )
			continue;

		dl_member_info_t* members = (dl_member_info_t*)malloc( type->member_count * sizeof( dl_member_info_t ) );
		dl_reflect_get_type_members( ctx, type->tid, members, type->member_count );

		if( type->is_union )
		{
			dl_binary_writer_write_string_fmt( writer, "enum %s_type\n"
													   "{\n", type->name );

			// ... generate an enum for union-members ...
			for( unsigned int member_index = 0; member_index < type->member_count; ++member_index )
			{
				dl_binary_writer_write_string_fmt( writer, "    %s_type_%s = 0x%08X", type->name, members[member_index].name, dl_internal_hash_string(members[member_index].name) );

				if( member_index < type->member_count - 1 )
					dl_binary_writer_write_string_fmt( writer, ",\n" );
				else
					dl_binary_writer_write_string_fmt( writer, "\n" );
			}

			dl_binary_writer_write_string_fmt( writer, "};\n\n" );
		}

		if( type->comment ) {
			dl_binary_writer_write_string_fmt( writer, "//%s\n", type->comment );
		}

		// ... the struct need manual align if the struct has higher align than any of its members ...
		unsigned int max_align = 0;
		for( unsigned int member_index = 0; member_index < type->member_count; ++member_index )
			// TODO: this might fail if the compiler is run as 32 bit, expose manual align from reflect.
			max_align = members[member_index].alignment > max_align ? members[member_index].alignment : max_align;

		if( max_align < type->alignment )
			dl_binary_writer_write_string_fmt( writer, "struct DL_ALIGN( %u ) %s\n{\n", type->alignment, type->name );
		else
			dl_binary_writer_write_string_fmt( writer, "struct %s\n{\n", type->name );

		dl_binary_writer_write_string_fmt( writer, "#if defined( __cplusplus )\n"
												   "    static const uint32_t TYPE_ID = 0x%08X;\n"
												   "#endif // defined( __cplusplus )\n\n", type->tid );

		if( type->is_union )
		{
			dl_binary_writer_write_string_fmt( writer, "    union\n"
													   "    {\n" );

			// TODO: better indent here!
			bool last_was_bf = false;
			for( unsigned int member_index = 0; member_index < type->member_count; ++member_index )
				dl_context_write_c_header_member( writer, ctx, members + member_index, &last_was_bf );
			dl_binary_writer_write_string_fmt( writer, "    } value;\n" );

			dl_binary_writer_write_string_fmt( writer, "    enum %s_type type;\n", type->name );
		}
		else
		{
			bool last_was_bf = false;
			for( unsigned int member_index = 0; member_index < type->member_count; ++member_index )
				dl_context_write_c_header_member( writer, ctx, members + member_index, &last_was_bf );
		}

		free( members );
		dl_binary_writer_write_string_fmt( writer, "};\n\n" );
	}

	free( type_info );
}

dl_error_t dl_context_write_type_library_c_header( dl_ctx_t dl_ctx, const char* module_name, char* out_header, size_t out_header_size, size_t* produced_bytes )
{
	char MODULE_NAME[128];
	size_t pos = 0;
	const char* iter = module_name;
	while( *iter && pos < 127 )
	{
		if( isalnum( *iter ) )
			MODULE_NAME[pos++] = (char)toupper( *iter );
		else
			MODULE_NAME[pos++] = '_';
		++iter;
	}
	MODULE_NAME[pos] = 0;

	dl_binary_writer writer;
	dl_binary_writer_init( &writer, (uint8_t*)out_header, out_header_size, out_header == 0x0, DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_HOST );

	dl_context_write_c_header_begin( &writer, MODULE_NAME );

	dl_context_write_c_header_typeids( &writer, dl_ctx );

	dl_context_write_c_header_enums( &writer, dl_ctx );

	dl_context_write_c_header_types( &writer, dl_ctx );

	dl_context_write_c_header_end( &writer, MODULE_NAME );

	if( produced_bytes )
		*produced_bytes = dl_binary_writer_needed_size( &writer );

	return DL_ERROR_OK;
}
