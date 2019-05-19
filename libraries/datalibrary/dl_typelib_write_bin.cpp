#include <dl/dl_typelib.h>
#include "dl_binary_writer.h"

dl_error_t dl_context_write_type_library( dl_ctx_t dl_ctx, unsigned char* out_lib, size_t out_lib_size, size_t* produced_bytes )
{
	dl_binary_writer writer;
	dl_binary_writer_init( &writer, out_lib, out_lib_size, out_lib == 0x0, DL_ENDIAN_HOST, DL_ENDIAN_HOST, DL_PTR_SIZE_32BIT );

	// ... write header ...
	// TODO: handle endianness!
	dl_typelib_header header;
	header.id         = DL_TYPELIB_ID;
	header.version    = DL_TYPELIB_VERSION;
	header.type_count = dl_ctx->type_count;
	header.enum_count = dl_ctx->enum_count;
	header.member_count = dl_ctx->member_count;
	header.enum_value_count = dl_ctx->enum_value_count;
	header.enum_alias_count = dl_ctx->enum_alias_count;
	header.default_value_size = (uint32_t)dl_ctx->default_data_size;
	header.typeinfo_strings_size = (uint32_t)dl_ctx->typedata_strings_size;

	dl_binary_writer_write( &writer, &header, sizeof( dl_typelib_header ) );
	if(dl_ctx->type_count) dl_binary_writer_write( &writer, dl_ctx->type_ids, sizeof( dl_typeid_t ) * dl_ctx->type_count );
	if(dl_ctx->enum_count) dl_binary_writer_write( &writer, dl_ctx->enum_ids, sizeof( dl_typeid_t ) * dl_ctx->enum_count );
	if(dl_ctx->type_count) dl_binary_writer_write( &writer, dl_ctx->type_descs, sizeof( dl_type_desc ) * dl_ctx->type_count );
	if(dl_ctx->enum_count) dl_binary_writer_write( &writer, dl_ctx->enum_descs, sizeof( dl_enum_desc ) * dl_ctx->enum_count );
	if(dl_ctx->member_count) dl_binary_writer_write( &writer, dl_ctx->member_descs, sizeof( dl_member_desc ) * dl_ctx->member_count );
	if(dl_ctx->enum_value_count) dl_binary_writer_write( &writer, dl_ctx->enum_value_descs, sizeof( dl_enum_value_desc ) * dl_ctx->enum_value_count );
	if(dl_ctx->enum_alias_count) dl_binary_writer_write( &writer, dl_ctx->enum_alias_descs, sizeof( dl_enum_alias_desc ) * dl_ctx->enum_alias_count );
	if(dl_ctx->default_data_size) dl_binary_writer_write( &writer, dl_ctx->default_data, dl_ctx->default_data_size );
	if(dl_ctx->typedata_strings_size) dl_binary_writer_write( &writer, dl_ctx->typedata_strings, dl_ctx->typedata_strings_size );

	// ... write default data ...

	if( produced_bytes )
		*produced_bytes = dl_binary_writer_needed_size( &writer );

	// TODO: should write buffer to small on error.
	return DL_ERROR_OK;
}
