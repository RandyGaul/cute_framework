/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include "dl_types.h"

#include <dl/dl_reflect.h>

// TODO: Write unittests for new functionality!!!

dl_error_t dl_reflect_context_info( dl_ctx_t dl_ctx, dl_type_context_info_t* info )
{
	info->num_types = dl_ctx->type_count;
	info->num_enums = dl_ctx->enum_count;
	return DL_ERROR_OK;
}

dl_error_t dl_reflect_loaded_typeids( dl_ctx_t dl_ctx, dl_typeid_t* out_types, unsigned int out_types_size )
{
	if( dl_ctx->type_count > out_types_size )
		return DL_ERROR_BUFFER_TO_SMALL;

	memcpy( out_types, dl_ctx->type_ids, sizeof( dl_typeid_t ) * dl_ctx->type_count );
	return DL_ERROR_OK;
}

dl_error_t dl_reflect_loaded_enumids( dl_ctx_t dl_ctx, dl_typeid_t* out_enums, unsigned int out_enums_size )
{
	if( dl_ctx->enum_count > out_enums_size )
		return DL_ERROR_BUFFER_TO_SMALL;

	memcpy( out_enums, dl_ctx->enum_ids, sizeof( dl_typeid_t ) * dl_ctx->enum_count );
	return DL_ERROR_OK;
}

static void dl_reflect_copy_type_info( dl_ctx_t ctx, dl_type_info_t* typeinfo, const dl_type_desc* type )
{
	typeinfo->tid           = ctx->type_ids[ type - ctx->type_descs ];
	typeinfo->name          = dl_internal_type_name( ctx, type );
	typeinfo->comment       = dl_internal_type_comment( ctx, type );
	typeinfo->size          = type->size[DL_PTR_SIZE_HOST];
	typeinfo->alignment     = type->alignment[DL_PTR_SIZE_HOST];
	typeinfo->member_count  = type->member_count;
	typeinfo->is_extern     = ( type->flags & DL_TYPE_FLAG_IS_EXTERNAL ) ? 1 : 0;
	typeinfo->is_union      = ( type->flags & DL_TYPE_FLAG_IS_UNION ) ? 1 : 0;
	typeinfo->should_verify = ( type->flags & DL_TYPE_FLAG_VERIFY_EXTERNAL_SIZE_ALIGN ) ? 1 : 0;
}

static void dl_reflect_copy_enum_info( dl_ctx_t ctx, dl_enum_info_t* enuminfo, const dl_enum_desc* enum_ )
{
	enuminfo->tid         = ctx->enum_ids[ enum_ - ctx->enum_descs ];
	enuminfo->name        = dl_internal_enum_name( ctx, enum_ );
	enuminfo->comment     = dl_internal_enum_comment( ctx, enum_ );
	enuminfo->storage     = enum_->storage;
	enuminfo->value_count = enum_->value_count;
	enuminfo->is_extern   = ( enum_->flags & DL_TYPE_FLAG_IS_EXTERNAL ) ? 1 : 0;
}

dl_error_t DL_DLL_EXPORT dl_reflect_loaded_types( dl_ctx_t dl_ctx, dl_type_info_t* out_types, unsigned int out_types_size )
{
	if( dl_ctx->type_count > out_types_size )
		return DL_ERROR_BUFFER_TO_SMALL;

	for( uint32_t i = 0; i < dl_ctx->type_count; ++i )
		dl_reflect_copy_type_info( dl_ctx, out_types + i, dl_ctx->type_descs + i );

	return DL_ERROR_OK;
}

dl_error_t dl_reflect_loaded_enums( dl_ctx_t dl_ctx, dl_enum_info_t* out_enums, unsigned int out_enums_size )
{
	if( dl_ctx->enum_count > out_enums_size )
		return DL_ERROR_BUFFER_TO_SMALL;

	for( uint32_t i = 0; i < dl_ctx->enum_count; ++i )
		dl_reflect_copy_enum_info( dl_ctx, out_enums + i, dl_ctx->enum_descs + i );

	return DL_ERROR_OK;
}

dl_error_t dl_reflect_get_type_id( dl_ctx_t dl_ctx, const char* type_name, dl_typeid_t* out_type_id )
{
	const dl_type_desc* type = dl_internal_find_type_by_name( dl_ctx, type_name );
	if(type == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	*out_type_id = dl_internal_typeid_of( dl_ctx, type );

	return DL_ERROR_OK;
}

dl_error_t dl_reflect_get_type_info( dl_ctx_t dl_ctx, dl_typeid_t type_id, dl_type_info_t* out_type )
{
	const dl_type_desc* type = dl_internal_find_type(dl_ctx, type_id);
	if(type == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	dl_reflect_copy_type_info( dl_ctx, out_type, type );
	return DL_ERROR_OK;
}

dl_error_t DL_DLL_EXPORT dl_reflect_get_enum_info( dl_ctx_t dl_ctx, dl_typeid_t type, dl_enum_info_t* out_enum_info )
{
	const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, type );
	if( e == 0x0 )
		return DL_ERROR_TYPE_NOT_FOUND;

	dl_reflect_copy_enum_info( dl_ctx, out_enum_info, e );
	return DL_ERROR_OK;
}

dl_error_t DL_DLL_EXPORT dl_reflect_get_type_members( dl_ctx_t dl_ctx, dl_typeid_t type_id, dl_member_info_t* out_members, unsigned int members_size )
{
	const dl_type_desc* type = dl_internal_find_type( dl_ctx, type_id );
	if(type == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	if(members_size < type->member_count)
		return DL_ERROR_BUFFER_TO_SMALL;

	for( uint32_t member_index = 0; member_index < type->member_count; ++member_index )
	{
		const dl_member_desc* member = dl_get_type_member( dl_ctx, type, member_index );

		out_members[member_index].name          = dl_internal_member_name( dl_ctx, member );
		out_members[member_index].comment       = dl_internal_member_comment( dl_ctx, member );
		out_members[member_index].atom          = member->AtomType();
		out_members[member_index].storage       = member->StorageType();
		out_members[member_index].type_id       = member->type_id;
		out_members[member_index].size          = member->size[DL_PTR_SIZE_HOST];
		out_members[member_index].alignment     = member->alignment[DL_PTR_SIZE_HOST];
		out_members[member_index].offset        = member->offset[DL_PTR_SIZE_HOST];
		out_members[member_index].array_count   = 0;
		out_members[member_index].bits          = 0;
		out_members[member_index].is_const      = ( member->flags & DL_MEMBER_FLAG_IS_CONST ) ? 1 : 0;
		out_members[member_index].should_verify = ( member->flags & DL_MEMBER_FLAG_VERIFY_EXTERNAL_SIZE_OFFSET ) ? 1 : 0;

		switch(member->AtomType())
		{
			case DL_TYPE_ATOM_INLINE_ARRAY:
				out_members[member_index].array_count = member->inline_array_cnt();
				break;
			case DL_TYPE_ATOM_BITFIELD:
				out_members[member_index].bits = member->bitfield_bits();
				break;
			default:
				break;
		}
	}

	return DL_ERROR_OK;
}

dl_error_t DL_DLL_EXPORT dl_reflect_get_enum_values( dl_ctx_t dl_ctx, dl_typeid_t type, dl_enum_value_info_t* out_values, unsigned int out_values_size )
{
	const dl_enum_desc* e = dl_internal_find_enum( dl_ctx, type );
	if( e == 0x0 ) return DL_ERROR_TYPE_NOT_FOUND;
	if( out_values_size < e->value_count ) return DL_ERROR_BUFFER_TO_SMALL;

	for( uint32_t value = 0; value < e->value_count; ++value )
	{
		const dl_enum_value_desc* v = dl_get_enum_value( dl_ctx, e, value );
		out_values[value].name  = dl_internal_enum_alias_name( dl_ctx, &dl_ctx->enum_alias_descs[v->main_alias]);
		out_values[value].value.u64 = v->value;
	}

	return DL_ERROR_OK;
}
