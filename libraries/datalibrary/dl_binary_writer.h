/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_BINARY_WRITER_H_INCLUDED
#define DL_DL_BINARY_WRITER_H_INCLUDED

#include <stdio.h>
#include "dl_types.h"

#if 0 // BinaryWriterVerbose
	#define DL_LOG_BIN_WRITER_VERBOSE(_Fmt, ...) printf("DL: " _Fmt "\n", ##__VA_ARGS__)
#else
	#define DL_LOG_BIN_WRITER_VERBOSE(_Fmt, ...)
#endif

struct dl_binary_writer
{
	bool          dummy;
	dl_endian_t   source_endian;
	dl_endian_t   target_endian;
	dl_ptr_size_t ptr_size;
	size_t        pos;
	size_t        needed_size;
	uint8_t*      data;
	size_t        data_size;
};

static inline void dl_binary_writer_init( dl_binary_writer* writer,
										  uint8_t* out_data, size_t out_data_size, bool dummy,
										  dl_endian_t source_endian, dl_endian_t target_endian,
										  dl_ptr_size_t target_ptr_size )
{
	writer->dummy          = dummy;
	writer->source_endian  = source_endian;
	writer->target_endian  = target_endian;
	writer->ptr_size       = target_ptr_size;
	writer->pos            = 0;
	writer->needed_size    = 0;
	writer->data           = out_data;
	writer->data_size      = out_data_size;
}

static inline void   dl_binary_writer_seek_set( dl_binary_writer* writer, size_t pos ) { writer->pos  = pos;                 DL_LOG_BIN_WRITER_VERBOSE("Seek Set: " DL_PINT_FMT_STR, writer->pos); }
static inline void   dl_binary_writer_seek_end( dl_binary_writer* writer )             { writer->pos  = writer->needed_size; DL_LOG_BIN_WRITER_VERBOSE("Seek End: " DL_PINT_FMT_STR, writer->pos); }
static inline size_t dl_binary_writer_tell( dl_binary_writer* writer )                 { return writer->pos; }
static inline size_t dl_binary_writer_needed_size( dl_binary_writer* writer )          { return writer->needed_size; }

static inline void dl_binary_writer_update_needed_size( dl_binary_writer* writer )
{
	writer->needed_size = writer->needed_size >= writer->pos ? writer->needed_size : writer->pos;
}

static inline void dl_binary_writer_write( dl_binary_writer* writer, const void* data, size_t size )
{
	if( !writer->dummy && ( writer->pos + size <= writer->data_size ) )
	{
		switch( size )
		{
			case 1: DL_LOG_BIN_WRITER_VERBOSE ("Write: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (%u)",                    writer->pos, size, *(char*)data   ); break;
			case 2: DL_LOG_BIN_WRITER_VERBOSE ("Write: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (%u)",                    writer->pos, size, *(uint16_t*)data ); break;
			case 4: DL_LOG_BIN_WRITER_VERBOSE ("Write: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (%u)",                    writer->pos, size, *(uint32_t*)data ); break;
			case 8: DL_LOG_BIN_WRITER_VERBOSE ("Write: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (" DL_UINT64_FMT_STR ")", writer->pos, size, *(uint64_t*)data ); break;
			default: DL_LOG_BIN_WRITER_VERBOSE("Write: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR, writer->pos, size ); break;
		}
		DL_ASSERT( writer->pos + size <= writer->data_size && "To small buffer!" );
		memmove( writer->data + writer->pos, data, size);
	}

	writer->pos += size;
	dl_binary_writer_update_needed_size( writer );
}

static inline void dl_binary_writer_write_int8  ( dl_binary_writer* writer, int8_t    u ) { dl_binary_writer_write( writer, &u, sizeof(int8_t) );    }
static inline void dl_binary_writer_write_int16 ( dl_binary_writer* writer, int16_t   u ) { dl_binary_writer_write( writer, &u, sizeof(int16_t) );   }
static inline void dl_binary_writer_write_int32 ( dl_binary_writer* writer, int32_t   u ) { dl_binary_writer_write( writer, &u, sizeof(int32_t) );   }
static inline void dl_binary_writer_write_int64 ( dl_binary_writer* writer, int64_t   u ) { dl_binary_writer_write( writer, &u, sizeof(int64_t) );   }
static inline void dl_binary_writer_write_uint8 ( dl_binary_writer* writer, uint8_t   u ) { dl_binary_writer_write( writer, &u, sizeof(uint8_t) );   }
static inline void dl_binary_writer_write_uint16( dl_binary_writer* writer, uint16_t  u ) { dl_binary_writer_write( writer, &u, sizeof(uint16_t) );  }
static inline void dl_binary_writer_write_uint32( dl_binary_writer* writer, uint32_t  u ) { dl_binary_writer_write( writer, &u, sizeof(uint32_t) );  }
static inline void dl_binary_writer_write_uint64( dl_binary_writer* writer, uint64_t  u ) { dl_binary_writer_write( writer, &u, sizeof(uint64_t) );  }
static inline void dl_binary_writer_write_fp32  ( dl_binary_writer* writer, float     u ) { dl_binary_writer_write( writer, &u, sizeof(float)    );  }
static inline void dl_binary_writer_write_fp64  ( dl_binary_writer* writer, double    u ) { dl_binary_writer_write( writer, &u, sizeof(double)   );  }
static inline void dl_binary_writer_write_pint  ( dl_binary_writer* writer, uintptr_t u ) { dl_binary_writer_write( writer, &u, sizeof(uintptr_t) ); }

static inline void dl_binary_writer_write_1byte( dl_binary_writer* writer, const void* data )
{
	dl_binary_writer_write( writer, data, 1 );
}

static inline void dl_binary_writer_write_2byte( dl_binary_writer* writer, const void* data )
{
	union { uint16_t* u16; const void* data; } conv;
	conv.data = data;
	uint16_t val = *conv.u16;

	if( writer->source_endian != writer->target_endian )
		val = dl_swap_endian_uint16( val );

	dl_binary_writer_write( writer, &val, 2 );
}

static inline void dl_binary_writer_write_4byte( dl_binary_writer* writer, const void* data )
{
	union { uint32_t* u32; const void* data; } conv;
	conv.data = data;
	uint32_t val = *conv.u32;

	if( writer->source_endian != writer->target_endian )
		val = dl_swap_endian_uint32( val );

	dl_binary_writer_write( writer, &val, 4 );
}

static inline void dl_binary_writer_write_8byte( dl_binary_writer* writer, const void* data )
{
	union { uint64_t* u64; const void* data; } conv;
	conv.data = data;
	uint64_t val = *conv.u64;

	if( writer->source_endian != writer->target_endian )
		val = dl_swap_endian_uint64( val );

	dl_binary_writer_write( writer, &val, 8 );
}

static inline void dl_binary_writer_write_swap( dl_binary_writer* writer, const void* data, size_t size )
{
	switch( size )
	{
		case 1: dl_binary_writer_write_1byte( writer, data ); break;
		case 2: dl_binary_writer_write_2byte( writer, data ); break;
		case 4: dl_binary_writer_write_4byte( writer, data ); break;
		case 8: dl_binary_writer_write_8byte( writer, data ); break;
		default:
			DL_ASSERT( false && "unhandled case!" );
			break;
	}
}

static inline void dl_binary_writer_write_array( dl_binary_writer* writer, const void* array, size_t count, size_t elem_size )
{
	if( !writer->dummy )
	{
		DL_LOG_BIN_WRITER_VERBOSE( "Write Array: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (" DL_PINT_FMT_STR ")", writer->pos, elem_size, count );
		for (size_t i = 0; i < count; ++i)
		{
			switch( elem_size )
			{
				case 1: { DL_LOG_BIN_WRITER_VERBOSE( "\t%c",                 ((const uint8_t*)  array)[i] ); } break;
				case 2: { DL_LOG_BIN_WRITER_VERBOSE( "\t%u",                 ((const uint16_t*) array)[i] ); } break;
				case 4: { DL_LOG_BIN_WRITER_VERBOSE( "\t%u",                 ((const uint32_t*) array)[i] ); } break;
				case 8: { DL_LOG_BIN_WRITER_VERBOSE( "\t" DL_UINT64_FMT_STR, ((const uint64_t*) array)[i] ); } break;
			}
		}
	}

	if( writer->source_endian != writer->target_endian )
	{
		for(size_t i = 0; i < count; ++i)
		{
			switch( elem_size )
			{
				case 1: dl_binary_writer_write_1byte( writer, (const uint8_t*)array + i * elem_size ); break;
				case 2: dl_binary_writer_write_2byte( writer, (const uint8_t*)array + i * elem_size ); break;
				case 4: dl_binary_writer_write_4byte( writer, (const uint8_t*)array + i * elem_size ); break;
				case 8: dl_binary_writer_write_8byte( writer, (const uint8_t*)array + i * elem_size ); break;
				default:
					DL_ASSERT( false && "unhandled case!" );
					break;
			}
		}
	}
	else
		dl_binary_writer_write( writer, array, elem_size * count );
}

// val is expected to be in host-endian!!!
static inline void dl_binary_writer_write_ptr( dl_binary_writer* writer, size_t val )
{
	if( writer->target_endian != DL_ENDIAN_HOST )
	{
		switch( writer->ptr_size )
		{
			case DL_PTR_SIZE_32BIT: { uint32_t u = (uint32_t)val; dl_binary_writer_write_4byte( writer, &u ); break; }
			case DL_PTR_SIZE_64BIT: { uint64_t u = (uint64_t)val; dl_binary_writer_write_8byte( writer, &u ); break; }
			default:
				DL_ASSERT(false && "Bad ptr-size!?!");
				break;
		}
	}
	else
	{
		switch( writer->ptr_size )
		{
			case DL_PTR_SIZE_32BIT: { uint32_t u = (uint32_t)val; dl_binary_writer_write( writer, &u, 4 ); } break;
			case DL_PTR_SIZE_64BIT: { uint64_t u = (uint64_t)val; dl_binary_writer_write( writer, &u, 8 ); } break;
			default:
				DL_ASSERT(false && "Bad ptr-size!?!");
				break;
		}
	}
}

static inline void dl_binary_writer_write_string( dl_binary_writer* writer, const void* str, size_t len )
{
	DL_LOG_BIN_WRITER_VERBOSE("Write string: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " = \"%.*s\"", writer->pos, len + 1, (int)len, (char*)str);
	dl_binary_writer_write( writer, str, len );
}

static inline void dl_binary_writer_write_zero( dl_binary_writer* writer, size_t bytes )
{
	if( !writer->dummy )
	{
		DL_LOG_BIN_WRITER_VERBOSE("Write zero: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR, writer->pos, bytes);
		DL_ASSERT( writer->pos + bytes <= writer->data_size && "To small buffer!" );
		memset( writer->data + writer->pos, 0x0, bytes );
	}

	writer->pos += bytes;
	dl_binary_writer_update_needed_size( writer );
}

static inline void dl_binary_writer_reserve( dl_binary_writer* writer, size_t bytes )
{
	DL_LOG_BIN_WRITER_VERBOSE( "Reserve: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR, writer->pos, bytes );
	writer->needed_size = writer->needed_size >= writer->pos + bytes ? writer->needed_size : writer->pos + bytes;
}

static inline uint8_t  dl_binary_writer_read_uint8 ( dl_binary_writer* writer ) { return writer->dummy ? ( uint8_t)0 : *( uint8_t*)( writer->data + writer->pos ); }
static inline uint16_t dl_binary_writer_read_uint16( dl_binary_writer* writer ) { return writer->dummy ? (uint16_t)0 : *(uint16_t*)( writer->data + writer->pos ); }
static inline uint32_t dl_binary_writer_read_uint32( dl_binary_writer* writer ) { return writer->dummy ? (uint32_t)0 : *(uint32_t*)( writer->data + writer->pos ); }
static inline uint64_t dl_binary_writer_read_uint64( dl_binary_writer* writer ) { return writer->dummy ? (uint64_t)0 : *(uint64_t*)( writer->data + writer->pos ); }

static inline void dl_binary_writer_align( dl_binary_writer* writer, size_t align )
{
	size_t alignment = dl_internal_align_up( writer->pos, align );
	if( !writer->dummy && alignment != writer->pos )
	{
		DL_LOG_BIN_WRITER_VERBOSE( "Align: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (" DL_PINT_FMT_STR ")", writer->pos, alignment - writer->pos, align );
		memset( writer->data + writer->pos, 0x0, alignment - writer->pos);
	}
	writer->pos = alignment;
	dl_binary_writer_update_needed_size( writer );
};

#endif // DL_DL_BINARY_WRITER_H_INCLUDED
