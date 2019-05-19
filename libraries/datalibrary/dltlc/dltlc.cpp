#include <dl/dl.h>
#include <dl/dl_typelib.h>
#include <dl/dl_reflect.h>

#include "getopt/getopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

struct dltlc_args
{
	const char* input;
	const char* output;

	int unpack;
	int show_info;
	int c_header;
};

static int verbose = 0;
std::vector<const char*> inputs;

#define VERBOSE_OUTPUT(fmt, ...) if( verbose ) { fprintf(stderr, fmt "\n", ##__VA_ARGS__); }


static int parse_args( int argc, const char** argv, dltlc_args* args )
{
	memset( args, 0x0, sizeof(dltlc_args) );

	const getopt_option_t option_list[] =
	{
		{ "help",     'h', GETOPT_OPTION_TYPE_NO_ARG,   0x0,            'h', "displays this help-message", 0x0 },
		{ "output",   'o', GETOPT_OPTION_TYPE_REQUIRED, 0x0,            'o', "output to file", "file" },
		{ "unpack",   'u', GETOPT_OPTION_TYPE_FLAG_SET, &args->unpack,    1, "force dl_pack to treat input data as a packed instance that should be unpacked.", 0x0 },
		{ "info",     'i', GETOPT_OPTION_TYPE_FLAG_SET, &args->show_info, 1, "make dl_pack show info about a packed instance.", 0x0 },
		{ "verbose",  'v', GETOPT_OPTION_TYPE_FLAG_SET, &verbose,         1, "verbose output", 0x0 },
		{ "c-header", 'c', GETOPT_OPTION_TYPE_FLAG_SET, &args->c_header,  1, "output c header instead of tld binary", 0x0 },
		GETOPT_OPTIONS_END
	};

	getopt_context_t go_ctx;
	getopt_create_context( &go_ctx, argc, argv, option_list );

	int opt;
	while( (opt = getopt_next( &go_ctx ) ) != -1 )
	{
		switch(opt)
		{
			case 0:
				/*ignore, flag was set*/
				break;

			case 'h':
			{
				char buffer[2048];
				printf("usage: dltlc.exe [options] [input]...\n\n");
				printf("%s", getopt_create_help_string( &go_ctx, buffer, sizeof(buffer) ) );
				return 0;
			}

			case 'o':
				// if( output != stdout )
				if( args->output != 0x0 )
				{
					fprintf( stderr, "specified -o/--output twice!\n" );
					return 1;
				}

				args->output = go_ctx.current_opt_arg;
				break;

			case '!':
				fprintf( stderr, "incorrect usage of flag \"%s\"\n", go_ctx.current_opt_arg );
				return 1;

			case '?':
				fprintf( stderr, "unknown flag \"%s\"\n", go_ctx.current_opt_arg );
				return 1;

			case '+':
				inputs.push_back( go_ctx.current_opt_arg );
				break;
		}
	}

	if( args->show_info + args->c_header + args->unpack > 1 )
	{
		fprintf( stderr, "more than one of, -u,--unpack, -i,--info or -c,--c_header was specified!\n" );
		return 1;
	}

	return 2;
}

static void error_report_function( const char* msg, void* )
{
	fprintf( stderr, "%s\n", msg );
}

static unsigned char* read_entire_stream( FILE* file, size_t* out_size )
{
	const unsigned int CHUNK_SIZE = 1024;
	size_t         total_size = 0;
	size_t         chunk_size = 0;
	unsigned char* out_buffer = 0;

	do
	{
		out_buffer = (unsigned char*)realloc( out_buffer, CHUNK_SIZE + total_size );
		chunk_size = fread( out_buffer + total_size, 1, CHUNK_SIZE, file );
		total_size += chunk_size;
	}
	while( chunk_size >= CHUNK_SIZE );

	*out_size = total_size;
	return out_buffer;
}

static bool load_typelib( dl_ctx_t ctx, FILE* f )
{
	size_t size = 0;
	unsigned char* data = read_entire_stream( f, &size );

	dl_error_t err = dl_context_load_type_library( ctx, data, size );
	if( err != DL_ERROR_OK )
		err = dl_context_load_txt_type_library( ctx, (const char*)data, size ); // ... try text ...

	free( data );

	if( err != DL_ERROR_OK )
		VERBOSE_OUTPUT( "failed to load typelib with error %s", dl_error_to_string( err ) );
	return err == DL_ERROR_OK;
}

static int write_tl_as_text( dl_ctx_t ctx, FILE* out )
{
	dl_error_t err;

	// ... query result size ...
	size_t res_size;
	err = dl_context_write_txt_type_library( ctx, 0x0, 0, &res_size );
	if( err != DL_ERROR_OK )
	{
		fprintf( stderr, "failed to query typelib-txt size with error \"%s\"\n", dl_error_to_string( err ) );
		return 1;
	}

	char* outdata = (char*)malloc( res_size );
	err = dl_context_write_txt_type_library( ctx, outdata, res_size, 0x0 );
	if( err == DL_ERROR_OK )
		fwrite( outdata, res_size, 1, out );
	else
		fprintf( stderr, "failed to write typelib-txt size with error \"%s\"\n", dl_error_to_string( err ) );

	free( outdata );
	return err == DL_ERROR_OK ? 0 : 1;
}

static int write_tl_as_c_header( dl_ctx_t ctx, const char* module_name, FILE* out )
{
	dl_error_t err;

	// ... query result size ...
	size_t res_size;
	err = dl_context_write_type_library_c_header( ctx, module_name, 0x0, 0, &res_size );
	if( err != DL_ERROR_OK )
	{
		fprintf( stderr, "failed to query c-header size for typelib with error \"%s\"\n", dl_error_to_string( err ) );
		return 1;
	}

	char* outdata = (char*)malloc( res_size );
	err = dl_context_write_type_library_c_header( ctx, module_name, outdata, res_size, 0x0 );
	if( err == DL_ERROR_OK )
		fwrite( outdata, res_size, 1, out );
	else
		fprintf( stderr, "failed to write c-header for typelib with error \"%s\"\n", dl_error_to_string( err ) );

	free( outdata );
	return err == DL_ERROR_OK ? 0 : 1;
}

static int write_tl_as_binary( dl_ctx_t ctx, FILE* out )
{
	dl_error_t err;

	// ... query result size ...
	size_t res_size;
	err = dl_context_write_type_library( ctx, 0x0, 0, &res_size );
	if( err != DL_ERROR_OK )
	{
		fprintf( stderr, "failed to query typelib size with error \"%s\"\n", dl_error_to_string( err ) );
		return 1;
	}

	unsigned char* outdata = (unsigned char*)malloc( res_size );
	err = dl_context_write_type_library( ctx, outdata, res_size, 0x0 );
	if( err == DL_ERROR_OK )
		fwrite( outdata, res_size, 1, out );
	else
		fprintf( stderr, "failed to write typelib size with error \"%s\"\n", dl_error_to_string( err ) );

	free( outdata );
	return err == DL_ERROR_OK ? 0 : 1;
}

static void show_tl_members( dl_ctx_t ctx, const char* member_fmt, dl_typeid_t tid, unsigned int member_count )
{
	dl_member_info_t* member_info = (dl_member_info_t*)malloc( member_count * sizeof( dl_member_info_t ) );
	dl_reflect_get_type_members( ctx, tid, member_info, member_count );
	for( unsigned int j = 0; j < member_count; ++j )
	{
//		const char* atom = "whoo";
//		switch( member_info->type & DL_TYPE_ATOM_MASK )
//		{
//			case DL_TYPE_ATOM_POD:          atom = ""; break;
//			case DL_TYPE_ATOM_ARRAY:        atom = "[]"; break;
//			case DL_TYPE_ATOM_INLINE_ARRAY: atom = "[x]"; break;
//			case DL_TYPE_ATOM_BITFIELD:     atom = " : bits"; break;
//		}

		// TODO: print name here!
		printf(member_fmt, member_info[j].name, member_info[j].size, member_info[j].alignment, member_info[j].offset);
	}
	free( member_info );
}

static void show_tl_info( dl_ctx_t ctx )
{
	dl_type_context_info_t ctx_info;
	dl_reflect_context_info( ctx, &ctx_info );

	// show info here :)
	printf("module name: %s\n", "<fetch the real name :)>" );
	printf("type count:  %u\n", ctx_info.num_types );
	printf("enum count:  %u\n\n", ctx_info.num_enums );

	size_t type_size = ctx_info.num_types * sizeof( dl_type_info_t );
	size_t enum_size = ctx_info.num_enums * sizeof( dl_enum_info_t );
	void* info_buffer = malloc( type_size > enum_size ? type_size : enum_size );

	dl_type_info_t* type_info = (dl_type_info_t*)info_buffer;
	dl_reflect_loaded_types( ctx, type_info, ctx_info.num_types );

	size_t max_name_len = 0;
	for( unsigned int i = 0; i < ctx_info.num_types; ++i )
	{
		size_t len = strlen( type_info[i].name );
		max_name_len = len > max_name_len ? len : max_name_len;
	}

	char header_fmt[256];
	char item_fmt[256];
	char member_fmt[256];
	snprintf( header_fmt, 256, "%-10s  %%-%lus %5s %s %s\n", "typeid", (long unsigned int)max_name_len, "size", "align", "offset" );
	snprintf( item_fmt,   256, "0x%%08X  %%-%lus %%5u %%5u\n", (long unsigned int)max_name_len );
	snprintf( member_fmt, 256, "   - %%-%lus        %%5u %%5u %%5u\n", (long unsigned int)max_name_len );

	printf("types:\n");
	int header_len = printf( header_fmt, "name");
	for( int i = 0; i < header_len - 1; ++i )
		printf( "-" );
	printf( "\n" );

	for( unsigned int i = 0; i < ctx_info.num_types; ++i )
	{
		dl_type_info_t* type = &type_info[i];
		// TODO: not only output data for host-platform =/
		printf( item_fmt, type->tid, type->name, type->size, type->alignment );
		show_tl_members( ctx, member_fmt, type->tid, type->member_count );
		printf("\n");
	}

	printf("\nenums:\n");
	for( int i = 0; i < header_len - 1; ++i )
		printf( "-" );
	printf( "\n" );

	dl_enum_info_t* enum_info = (dl_enum_info_t*)info_buffer;
	dl_reflect_loaded_enums( ctx, enum_info, ctx_info.num_enums );

	max_name_len = 0;
	for( unsigned int i = 0; i < ctx_info.num_enums; ++i )
	{
		dl_enum_info_t* enum_ = &enum_info[i];

		printf("%s\n", enum_->name);

		dl_enum_value_info_t* values = (dl_enum_value_info_t*)malloc( enum_->value_count * sizeof( dl_enum_value_info_t ) );
		dl_reflect_get_enum_values( ctx, enum_->tid, values, enum_->value_count );

		for( unsigned int j = 0; j < enum_->value_count; ++j )
			printf("    %s = %u\n", values[j].name, values[j].value.u32); // TODO: fix me.

		free( values );
	}

	free( info_buffer );
}

int main( int argc, const char** argv )
{
	dltlc_args args;
	int ret = parse_args( argc, argv, &args );
	if( ret < 2 )
		return ret;

	dl_ctx_t ctx;
	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);
	p.error_msg_func = error_report_function;

	dl_context_create( &ctx, &p );

	if( inputs.size() == 0 )
	{
		VERBOSE_OUTPUT( "loading typelib from stdin" );

		if( !load_typelib( ctx, stdin ) )
		{
			return 1;
		}
	}
	else
	{
		for( size_t i = 0; i < inputs.size(); ++i )
		{
			FILE* f = fopen( inputs[i], "rb" );
			if( f == 0x0 )
			{
				fprintf( stderr, "failed to open \"%s\"\n", inputs[i] );
				return 1;
			}

			if( !load_typelib( ctx, f ) )
			{
				fprintf( stderr, "failed to load typelib from \"%s\"\n", inputs[i] );
				fclose( f );
				return 1;
			}

			fclose( f );
		}
	}

	int res = 0;

	FILE* output = args.output == 0x0 ? stdout : fopen( args.output, "wb" );
	if( output == 0x0 )
	{
		fprintf( stderr, "failed to open output: \"%s\"\n", args.output );
		return 1;
	}

	if( args.show_info )
		show_tl_info( ctx );
	else if( args.unpack )
		res = write_tl_as_text( ctx, output );
	else if( args.c_header )
	{
		if( output == stdout )
			res = write_tl_as_c_header( ctx, "STDOUT", output );
		else
		{
			const char* module_name = args.output;
			const char* iter = args.output;
			while( *iter )
			{
				if( *iter == '/' || *iter == '\\' )
					module_name = iter + 1;
				++iter;
			}
			res = write_tl_as_c_header( ctx, module_name, output );
		}
	}
	else
		res = write_tl_as_binary( ctx, output );

	if( output != stdout )
		fclose( output );

	dl_context_destroy( ctx );

	return res;
}
