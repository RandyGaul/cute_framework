/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl_util.h>
#include <dl/dl_txt.h>
#include <dl/dl_convert.h>
#include "dl_alloc.h"
#include "dl_types.h"

#include <stdio.h>

static unsigned char* dl_read_entire_stream( dl_allocator *allocator, FILE* file, size_t* out_size )
{
	const unsigned int CHUNK_SIZE = 1024;
	size_t         total_size = 0;
	size_t         chunk_size = 0;
	unsigned char* out_buffer = 0;

	do
	{
		out_buffer = (unsigned char*)dl_realloc( allocator, out_buffer, CHUNK_SIZE + total_size, total_size );
		chunk_size = fread( out_buffer + total_size, 1, CHUNK_SIZE, file );
		total_size += chunk_size;
	}
	while( chunk_size >= CHUNK_SIZE );

	*out_size = total_size;
	return out_buffer;
}

dl_error_t dl_util_load_from_file( dl_ctx_t dl_ctx,     		dl_typeid_t         type,
                                   const char* filename,     	dl_util_file_type_t filetype,
                                   void**      out_instance, 	dl_typeid_t*        out_type,
                                   dl_allocator * allocator )
{
	dl_error_t error = DL_ERROR_UTIL_FILE_NOT_FOUND;

	FILE* in_file = fopen( filename, "rb" );

	if( in_file != 0x0 )
	{
		error = dl_util_load_from_stream( dl_ctx, type, in_file, filetype, out_instance, out_type, 0x0, allocator );
		fclose(in_file);
	}

	return error;
}

dl_error_t dl_util_load_from_stream( dl_ctx_t dl_ctx,       	dl_typeid_t         type,
									 FILE*    stream,       	dl_util_file_type_t filetype,
									 void**   out_instance, 	dl_typeid_t*        out_type,
									 size_t*  consumed_bytes, 	dl_allocator *allocator )
{
	dl_allocator mallocator;
	if(allocator == 0x0) {
		dl_allocator_initialize(&mallocator, 0x0, 0x0, 0x0, 0x0);
		allocator = &mallocator;
	}


	// TODO: this function need to handle alignment for _ppInstance
	(void)consumed_bytes; // TODO: Return good stuff here!
	size_t file_size;
	unsigned char* file_content = dl_read_entire_stream( allocator, stream, &file_size );

	file_content[file_size] = '\0';

	dl_error_t error = DL_ERROR_OK;
	dl_instance_info_t info;

	error = dl_instance_get_info( file_content, file_size, &info );

	dl_util_file_type_t in_file_type = error == DL_ERROR_OK ? DL_UTIL_FILE_TYPE_BINARY : DL_UTIL_FILE_TYPE_TEXT;

	if( ( in_file_type & filetype ) == 0 )
	{
		dl_free( allocator, file_content );
		return DL_ERROR_UTIL_FILE_TYPE_MISMATCH;
	}

	unsigned char* load_instance = 0x0;
	size_t         load_size = 0;

	switch(in_file_type)
	{
		case DL_UTIL_FILE_TYPE_BINARY:
		{
			if( type == 0 ) // autodetect filetype
				type = info.root_type;

			error = dl_convert( dl_ctx, type, file_content, file_size, 0x0, 0, DL_ENDIAN_HOST, sizeof(void*), &load_size );

			if( error != DL_ERROR_OK ) { dl_free( allocator, file_content ); return error; }

			// convert if needed
			if( load_size > file_size || info.ptrsize < sizeof(void*) )
			{
				load_instance = (unsigned char*)dl_alloc( allocator, load_size );

				error = dl_convert( dl_ctx, type, file_content, file_size, load_instance, load_size, DL_ENDIAN_HOST, sizeof(void*), 0x0 );

				dl_free( allocator, file_content );
			}
			else
			{
				load_instance = file_content;
				load_size     = file_size;
				error = dl_convert_inplace( dl_ctx, type, load_instance, load_size, DL_ENDIAN_HOST, sizeof(void*), 0x0 );
			}

			if( error != DL_ERROR_OK ) { dl_free( allocator, load_instance ); return error; }
		}
		break;
		case DL_UTIL_FILE_TYPE_TEXT:
		{
			// calc needed space
			size_t packed_size = 0;
			error = dl_txt_pack( dl_ctx, (char*)file_content, 0x0, 0, &packed_size );

			if(error != DL_ERROR_OK) { dl_free( allocator, file_content); return error; }

			load_instance = (unsigned char*)dl_alloc( allocator, packed_size );

			error = dl_txt_pack(dl_ctx, (char*)file_content, load_instance, packed_size, 0x0);

			load_size = packed_size;

			dl_free( allocator, file_content);

			if(error != DL_ERROR_OK) { dl_free( allocator, load_instance); return error; }

			if( type == 0 ) // autodetect type
			{
				dl_instance_get_info( load_instance, packed_size, &info);
				type = info.root_type;
			}
		}
		break;
		default:
			return DL_ERROR_INTERNAL_ERROR;
	}

	error = dl_instance_load( dl_ctx, type, load_instance, load_size, load_instance, load_size, 0x0 );

	*out_instance = load_instance;

	if( out_type != 0x0 )
		*out_type = type;

	return error;
}

dl_error_t dl_util_load_from_file_inplace( dl_ctx_t    dl_ctx,       dl_typeid_t         type,
                                           const char* filename,     dl_util_file_type_t filetype,
                                           void*       out_instance, size_t              out_instance_size )
{
	(void)dl_ctx; (void)filename; (void)type; (void)filetype; (void)out_instance; (void)out_instance_size;
	return DL_ERROR_INTERNAL_ERROR; // TODO: Build me
}

dl_error_t dl_util_store_to_file( dl_ctx_t    dl_ctx,     dl_typeid_t         type,
                                  const char* filename,   dl_util_file_type_t filetype,
                                  dl_endian_t out_endian, size_t              out_ptr_size,
                                  const void* instance, dl_allocator *allocator /*= 0x0*/ )
{
	FILE* out_file = fopen( filename, filetype == DL_UTIL_FILE_TYPE_BINARY ? "wb" : "w" );

	dl_error_t error = DL_ERROR_UTIL_FILE_NOT_FOUND;

	if( out_file != 0x0 )
	{
		error = dl_util_store_to_stream( dl_ctx, type, out_file, filetype, out_endian, out_ptr_size, instance, allocator );
		fclose( out_file );
	}

	return error;
}

dl_error_t dl_util_store_to_stream( dl_ctx_t    dl_ctx,     dl_typeid_t         type,
									FILE*       stream,     dl_util_file_type_t filetype,
									dl_endian_t out_endian, size_t              out_ptr_size,
									const void* instance, dl_allocator *allocator )
{
	dl_allocator mallocator;
	if(allocator == 0x0) {
		dl_allocator_initialize(&mallocator, 0x0, 0x0, 0x0, 0x0);
		allocator = &mallocator;
	}

	if( filetype == DL_UTIL_FILE_TYPE_AUTO )
		return DL_ERROR_INVALID_PARAMETER;

	size_t packed_size = 0;

	// calculate pack-size
	dl_error_t error = dl_instance_store( dl_ctx, type, instance, 0x0, 0, &packed_size );

	if( error != DL_ERROR_OK)
		return error;

	// alloc memory
	unsigned char* packed_instance = (unsigned char*)dl_alloc( allocator, packed_size );

	// pack data
	error = dl_instance_store( dl_ctx, type, instance, packed_instance, packed_size, 0x0 );

	if( error != DL_ERROR_OK ) { dl_free( allocator, packed_instance ); return error; }

	size_t         out_size = 0;
	unsigned char* out_data = 0x0;

	switch( filetype )
	{
		case DL_UTIL_FILE_TYPE_BINARY:
		{
			// calc convert size
			error = dl_convert( dl_ctx, type, packed_instance, packed_size, 0x0, 0, out_endian, out_ptr_size, &out_size );

			if( error != DL_ERROR_OK ) { dl_free( allocator, packed_instance ); return error; }

			// convert
			if( out_size > packed_size || out_ptr_size > sizeof(void*) )
			{
				// new alloc
				out_data = (unsigned char*)dl_alloc( allocator, out_size );

				// convert
				error = dl_convert( dl_ctx, type, packed_instance, packed_size, out_data, out_size, out_endian, out_ptr_size, 0x0 );

				dl_free( allocator, packed_instance );

				if( error != DL_ERROR_OK ) { dl_free( allocator, out_data ); return error; }
			}
			else
			{
				out_data = packed_instance;
				error = dl_convert_inplace( dl_ctx, type, packed_instance, packed_size, out_endian, out_ptr_size, 0x0 );

				if( error != DL_ERROR_OK ) { dl_free( allocator, out_data ); return error; }
			}
		}
		break;
		case DL_UTIL_FILE_TYPE_TEXT:
		{
			// calculate pack-size
			error = dl_txt_unpack( dl_ctx, type, packed_instance, packed_size, 0x0, 0, &out_size );

			if( error != DL_ERROR_OK ) { dl_free( allocator, packed_instance ); return error; }

			// alloc data
			out_data = (unsigned char*)dl_alloc( allocator, out_size );

			// pack data
			error = dl_txt_unpack( dl_ctx, type, packed_instance, packed_size, (char*)out_data, out_size, 0x0 );

			dl_free( allocator, packed_instance );

			if( error != DL_ERROR_OK ) { dl_free( allocator, out_data ); return error; }
		}
		break;
		default:
			return DL_ERROR_INTERNAL_ERROR;
	}

	fwrite( out_data, out_size, 1, stream );
	dl_free( allocator,  out_data );

	return error;
}
