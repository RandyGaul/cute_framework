
#include <dl/dl.h>
#include <dl/dl_util.h>
#include <dl/dl_reflect.h>

#include "getopt/getopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M_VERBOSE_OUTPUT(fmt, ...) if(g_Verbose) { fprintf(stderr, fmt "\n", ##__VA_ARGS__); }
#define M_ERROR_AND_FAIL(fmt, ...) { fprintf(stderr, "Error: " fmt "\n", ##__VA_ARGS__); return 0x0; }
#define M_ERROR_AND_QUIT(fmt, ...) { fprintf(stderr, "Error: " fmt "\n", ##__VA_ARGS__); return 1; }

/*
	Tool that take DL-data in text-form and output a packed binary.
*/

int g_Verbose = 0;

enum
{
	MAX_LIB_PATHS = 128,
	MAX_LIBS      = 128
};

unsigned int g_num_lib_paths = 0;
const char*  g_lib_paths[MAX_LIB_PATHS];

unsigned int g_num_libs = 0;
const char*  g_libs[MAX_LIBS];

bool add_lib_path( const char* path )
{
	if( g_num_lib_paths >= MAX_LIB_PATHS )
		return false;
	g_lib_paths[g_num_lib_paths++] = path;
	return true;
}

bool add_lib( const char* lib )
{
	if( g_num_lib_paths > MAX_LIBS )
		return false;

	// TODO: handle to many paths here!
	g_libs[g_num_libs++] = lib;
	return true;
}

void error_report_function( const char* msg, void* ctx )
{
	(void)ctx;
	fprintf( stderr, "%s\n", msg );
}

void print_help( getopt_context_t* ctx )
{
	char buffer[2048];
	printf("usage: dl_pack.exe [options] file_to_pack\n\n");
	printf("%s", getopt_create_help_string( ctx, buffer, sizeof(buffer) ) );
}

unsigned char* read_file(FILE* file, size_t* out_size)
{
	const unsigned int CHUNK_SIZE = 1024;
	size_t         total_size = 0;
	size_t         chunk_size = 0;
	unsigned char* out_buffer = 0;

	do
	{
		out_buffer = (unsigned char*)realloc(out_buffer, CHUNK_SIZE + total_size);
		chunk_size = fread( out_buffer + total_size, 1, CHUNK_SIZE, file );
		total_size += chunk_size;
	}
	while( chunk_size >= CHUNK_SIZE );

	*out_size = total_size;
	return out_buffer;
}

dl_ctx_t create_ctx()
{
	dl_ctx_t dl_ctx;
	dl_create_params_t p;
	DL_CREATE_PARAMS_SET_DEFAULT(p);
	p.error_msg_func = error_report_function;
	dl_error_t err = dl_context_create( &dl_ctx, &p );
	if(err != DL_ERROR_OK)
		M_ERROR_AND_FAIL( "DL error while creating context: %s", dl_error_to_string(err) );

	// load all type-libs.
	for( unsigned int lib_index = 0; lib_index < g_num_libs; lib_index++ )
	{
		// search for lib!
		for ( unsigned int path_index = 0; path_index < g_num_lib_paths; ++path_index )
		{
			// build testpath.
			char   path[2048];
			size_t path_len = strlen( g_lib_paths[ path_index ] );
			strcpy( path, g_lib_paths[ path_index ] );
			if( path_len != 0 && path[path_len - 1] != '/' )
				path[ path_len++ ] = '/';
			strcpy( path + path_len, g_libs[ lib_index ] );

			FILE* file = fopen( path, "rb" );

			if( file != 0x0 )
			{
				M_VERBOSE_OUTPUT("Reading type-library from file %s", path);

				size_t         file_size;
				unsigned char* file_data = read_file( file, &file_size );
				err = dl_context_load_type_library( dl_ctx, file_data, file_size );
				free(file_data);
				fclose(file);
				if( err != DL_ERROR_OK )
					M_ERROR_AND_FAIL( "DL error while loading type library (%s): %s", path, dl_error_to_string(err) );
				break;
			}
		}
	}

	return dl_ctx;
}

int main( int argc, const char** argv )
{
	int show_info = 0;
	int do_unpack = 0;

	static const getopt_option_t option_list[] =
	{
		{ "help",    'h', GETOPT_OPTION_TYPE_NO_ARG,   0x0,        'h', "displays this help-message", 0x0 },
		{ "libpath", 'L', GETOPT_OPTION_TYPE_REQUIRED, 0x0,        'L', "add type-library include path", "path" },
		{ "lib",     'l', GETOPT_OPTION_TYPE_REQUIRED, 0x0,        'l', "add type-library", "path" },
		{ "output",  'o', GETOPT_OPTION_TYPE_REQUIRED, 0x0,        'o', "output to file", "file" },
		{ "endian",  'e', GETOPT_OPTION_TYPE_REQUIRED, 0x0,        'e', "endianness of output data, if not specified pack-platform is assumed", "little,big" },
		{ "ptrsize", 'p', GETOPT_OPTION_TYPE_REQUIRED, 0x0,        'p', "ptr-size of output data, if not specified pack-platform is assumed", "4,8" },
		{ "unpack",  'u', GETOPT_OPTION_TYPE_FLAG_SET, &do_unpack,   1, "force dl_pack to treat input data as a packed instance that should be unpacked.", 0x0 },
		{ "info",    'i', GETOPT_OPTION_TYPE_FLAG_SET, &show_info,   1, "make dl_pack show info about a packed instance.", 0x0 },
		{ "verbose", 'v', GETOPT_OPTION_TYPE_FLAG_SET, &g_Verbose,   1, "verbose output", 0x0 },
		GETOPT_OPTIONS_END
	};

	getopt_context_t go_ctx;
	getopt_create_context( &go_ctx, argc, argv, option_list );

	add_lib_path("");
	const char*  out_file_path  = "";
	const char*  in_file_path   = "";
	dl_endian_t  out_endian     = DL_ENDIAN_HOST;
	unsigned int out_ptr_size   = sizeof(void*);

	int opt;
	while( (opt = getopt_next( &go_ctx ) ) != -1 )
	{
		switch(opt)
		{
			case 'h': print_help(&go_ctx); return 0;
			case 'L': if( !add_lib_path( go_ctx.current_opt_arg ) ) M_ERROR_AND_QUIT( "dl_pack only supports %u libpaths!", MAX_LIB_PATHS );       break;
			case 'l': if( !add_lib( go_ctx.current_opt_arg ) )      M_ERROR_AND_QUIT( "dl_pack only supports %u type libraries libs!", MAX_LIBS ); break;
			case 'o':
				if(out_file_path[0] != '\0')
					M_ERROR_AND_QUIT("output-file already set to: \"%s\", trying to set it to \"%s\"", out_file_path, go_ctx.current_opt_arg);

				out_file_path = go_ctx.current_opt_arg;
				break;
			case 'e':
				if(strcmp(go_ctx.current_opt_arg, "little") == 0)
					out_endian = DL_ENDIAN_LITTLE;
				else if(strcmp(go_ctx.current_opt_arg, "big") == 0)
					out_endian = DL_ENDIAN_BIG;
				else
					M_ERROR_AND_QUIT("endian-flag need \"little\" or \"big\", not \"%s\"!", go_ctx.current_opt_arg);
				break;
			case 'p':
				if(strlen(go_ctx.current_opt_arg) != 1 || (go_ctx.current_opt_arg[0] != '4' && go_ctx.current_opt_arg[0] != '8'))
					M_ERROR_AND_QUIT("ptr-flag need \"4\" or \"8\", not \"%s\"!", go_ctx.current_opt_arg);

				out_ptr_size = (unsigned int)(go_ctx.current_opt_arg[0] - '0');
				break;
			case '!': M_ERROR_AND_QUIT("incorrect usage of flag \"%s\"!", go_ctx.current_opt_arg); break;
			case '?': M_ERROR_AND_QUIT("unrecognized flag \"%s\"!", go_ctx.current_opt_arg); break;
			case '+':
				if(in_file_path[0] != '\0')
					M_ERROR_AND_QUIT("input-file already set to: \"%s\", trying to set it to \"%s\"", in_file_path, go_ctx.current_opt_arg);

				in_file_path = go_ctx.current_opt_arg;
				break;
			case 0: break; // ignore, flag was set!
		}
	}

	FILE* in_file  = in_file_path[0]  == '\0' ? stdin  : fopen( in_file_path, "rb" );
	FILE* out_file = out_file_path[0] == '\0' ? stdout : fopen( out_file_path, "wb" );
	if( in_file  == 0x0 ) M_ERROR_AND_QUIT( "Could not open input file: %s", in_file_path );
	if( out_file == 0x0 ) M_ERROR_AND_QUIT( "Could not open output file: %s", out_file_path );

	dl_ctx_t dl_ctx = create_ctx();
	if( dl_ctx == 0x0 )
		return 1;

	if( show_info == 1 ) // show info about instance plox!
	{
		size_t         size;
		unsigned char* data = read_file( in_file, &size );

		dl_instance_info_t info;
		dl_instance_get_info( data, size, &info );

		dl_type_info_t tinfo;
		dl_reflect_get_type_info( dl_ctx, info.root_type, &tinfo );

		printf( "instance info:\n" );
		printf( "ptr size: %u\n",         info.ptrsize );
		printf( "endian:   %s\n",         info.endian == DL_ENDIAN_LITTLE ? "little" : "big" );
		printf( "type:     %s (0x%8X)\n", tinfo.name, info.root_type );

		free( data );
	}
	else
	{
		dl_typeid_t type;
		void* instance = 0;

		dl_error_t err = dl_util_load_from_stream( dl_ctx, 0, in_file, DL_UTIL_FILE_TYPE_AUTO, &instance, &type, 0x0, 0x0 );
		if( err != DL_ERROR_OK )
			M_ERROR_AND_QUIT( "DL error reading stream: %s", dl_error_to_string( err ) );

		err = dl_util_store_to_stream( dl_ctx,
									   type,
									   out_file,
									   do_unpack == 1 ? DL_UTIL_FILE_TYPE_TEXT : DL_UTIL_FILE_TYPE_BINARY,
									   out_endian,
									   out_ptr_size,
									   instance,
									   0x0 );

		if( err != DL_ERROR_OK )
			M_ERROR_AND_QUIT( "DL error writing stream: %s", dl_error_to_string( err ) );

		free( instance );
	}

	if( in_file_path[0]  != '\0' ) fclose( in_file );
	if( out_file_path[0] != '\0' ) fclose( out_file );

	dl_context_destroy( dl_ctx );

	return 0;
}
