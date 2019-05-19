/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include "dl_types.h"
#include "dl_binary_writer.h"
#include "container/dl_array.h"

#include <dl/dl.h>
#include <dl/dl_convert.h>

struct SInstance
{
	SInstance()
		: address(0x0)
		, array_count(0)
		, offset_after_patch(0)
		, type(0x0)
		, type_id(DL_TYPE_ATOM_MIN_BIT)
		{}
	SInstance( const uint8_t* address, const dl_type_desc* type, size_t array_count, dl_type_t tid )
		: address(address)
		, array_count(array_count)
		, offset_after_patch(0)
		, type(type)
		, type_id(tid)
		{ }

	const uint8_t*      address;
	size_t              array_count;
	uintptr_t           offset_after_patch;
	const dl_type_desc* type;
	dl_type_t           type_id;
};

class SConvertContext
{
public:
	SConvertContext( dl_endian_t src_endian, dl_endian_t tgt_endian, dl_ptr_size_t src_ptr_size, dl_ptr_size_t tgt_ptr_size )
		: src_endian(src_endian)
		, tgt_endian(tgt_endian)
		, src_ptr_size(src_ptr_size)
		, target_ptr_size(tgt_ptr_size)
	{}

	bool IsSwapped( const uint8_t* ptr )
	{
		for(unsigned int i = 0; i < instances.Len(); ++i)
			if(ptr == instances[i].address)
				return true;
		return false;
	}

	dl_endian_t src_endian;
	dl_endian_t tgt_endian;
	dl_ptr_size_t src_ptr_size;
	dl_ptr_size_t target_ptr_size;

	CArrayStatic<SInstance, 128> instances;

	struct PatchPos
	{
		PatchPos() : pos(0), old_offset(0) {}
		PatchPos(uintptr_t pos, uintptr_t old_offset)
			: pos(pos)
			, old_offset(old_offset)
		{ }
		uintptr_t pos;
		uintptr_t old_offset;
	};

	CArrayStatic<PatchPos, 256> m_lPatchOffset;
};

static inline void dl_swap_header( dl_data_header* header )
{
	header->id                 = dl_swap_endian_uint32( header->id );
	header->version            = dl_swap_endian_uint32( header->version );
	header->root_instance_type = dl_swap_endian_uint32( header->root_instance_type );
	header->instance_size      = dl_swap_endian_uint32( header->instance_size );
}

static uintptr_t dl_internal_read_ptr_data( const uint8_t* data,
								            dl_endian_t    src_endian,
								            dl_ptr_size_t  ptr_size )
{
	switch(ptr_size)
	{
		case DL_PTR_SIZE_32BIT:
		{
			uint32_t offset = *(uint32_t*)data;
			if(src_endian != DL_ENDIAN_HOST)
				return (uintptr_t)dl_swap_endian_uint32( offset );
			return (uintptr_t)offset;
		}
		break;
		case DL_PTR_SIZE_64BIT:
		{
			uint64_t offset = *(uint64_t*)data;
			if(src_endian != DL_ENDIAN_HOST)
				return (uintptr_t)dl_swap_endian_uint64( offset );
			return (uintptr_t)offset;
		}
		break;
		default:
			DL_ASSERT(false && "Invalid ptr-size");
			break;
	}

	return 0;
}

static size_t dl_internal_ptr_size(dl_ptr_size_t size_enum)
{
	switch(size_enum)
	{
		case DL_PTR_SIZE_32BIT: return 4;
		case DL_PTR_SIZE_64BIT: return 8;
		default: DL_ASSERT(false, "unknown ptr size!"); return 0;
	}
}

static void dl_internal_read_array_data( const uint8_t* array_data,
										 uintptr_t*     offset,
										 uint32_t*      count,
										 dl_endian_t    source_endian,
										 dl_ptr_size_t  ptr_size )
{
	union { const uint8_t* u8; const uint32_t* u32; const uint64_t* u64; };
	u8 = array_data;

	switch( ptr_size )
	{
		case DL_PTR_SIZE_32BIT:
		{
			if( source_endian != DL_ENDIAN_HOST )
			{
				*offset = (uintptr_t)dl_swap_endian_uint32( u32[0] );
				*count  =            dl_swap_endian_uint32( u32[1] );
			}
			else
			{
				*offset = (uintptr_t)(u32[0]);
				*count  = u32[1];
			}
		}
		break;

		case DL_PTR_SIZE_64BIT:
		{
			if( source_endian != DL_ENDIAN_HOST )
			{
				*offset = (uintptr_t)dl_swap_endian_uint64( u64[0] );
				*count  =            dl_swap_endian_uint32( u32[2] );
			}
			else
			{
				*offset = (uintptr_t)(u64[0]);
				*count  = u32[2];
			}
		}
		break;

		default:
			DL_ASSERT(false && "Invalid ptr-size");
			break;
	}
}

dl_type_t dl_make_type( dl_type_atom_t atom, dl_type_storage_t storage )
{
	return (dl_type_t)( ((unsigned int)atom << DL_TYPE_ATOM_MIN_BIT) | ((unsigned int)storage << DL_TYPE_STORAGE_MIN_BIT) );
}

static dl_error_t dl_internal_convert_collect_instances( dl_ctx_t            dl_ctx,
														 const dl_type_desc* type,
														 const uint8_t*      instance,
														 const uint8_t*      base_data,
														 SConvertContext&    convert_ctx );

static void dl_internal_convert_collect_instances_from_str( const uint8_t*        member_data,
																  const uint8_t*        base_data,
																  SConvertContext&      convert_ctx )
{
	uintptr_t offset = dl_internal_read_ptr_data( member_data, convert_ctx.src_endian, convert_ctx.src_ptr_size );
	if(offset != DL_NULL_PTR_OFFSET[convert_ctx.src_ptr_size])
		convert_ctx.instances.Add(SInstance(base_data + offset, 0x0, 1337, dl_make_type(DL_TYPE_ATOM_POD, DL_TYPE_STORAGE_STR)));
}

static void dl_internal_convert_collect_instances_from_ptr( dl_ctx_t              ctx,
															const dl_type_desc*   sub_type,
															const uint8_t*        member_data,
															const uint8_t*        base_data,
															SConvertContext&      convert_ctx )
{
	uintptr_t offset = dl_internal_read_ptr_data(member_data, convert_ctx.src_endian, convert_ctx.src_ptr_size);

	if(offset != DL_NULL_PTR_OFFSET[convert_ctx.src_ptr_size])
	{
		const uint8_t* ptr_data = base_data + offset;
		if(!convert_ctx.IsSwapped(ptr_data))
		{
			convert_ctx.instances.Add(SInstance(ptr_data, sub_type, 0, dl_make_type(DL_TYPE_ATOM_POD, DL_TYPE_STORAGE_PTR)));
			dl_internal_convert_collect_instances(ctx, sub_type, base_data + offset, base_data, convert_ctx);
		}
	}
}

static void dl_internal_convert_collect_instances_from_struct_array( dl_ctx_t            ctx,
																	 const uint8_t*      array_data,
																	 uintptr_t           array_count,
																	 const dl_type_desc* sub_type,
																	 const uint8_t*      base_data,
																	 SConvertContext&    convert_ctx )
{
	uint32_t elem_size = sub_type->size[convert_ctx.src_ptr_size];
	for( uint32_t elem = 0; elem < array_count; ++elem )
		dl_internal_convert_collect_instances(ctx, sub_type, array_data + (elem * elem_size), base_data, convert_ctx);
}

static void dl_internal_convert_collect_instances_from_str_array( const uint8_t*   array_data,
																  uintptr_t        array_count,
																  const uint8_t*   base_data,
																  SConvertContext& convert_ctx )
{
	// TODO: This might be optimized if we look at all the data in i inline-array of strings as 1 instance continuous in memory.
	// I am not sure if that is true for all cases right now!

	uint32_t ptr_size = (uint32_t)dl_internal_ptr_size(convert_ctx.src_ptr_size);

	for( uintptr_t elem = 0; elem < array_count; ++elem )
		dl_internal_convert_collect_instances_from_str( array_data + (elem * ptr_size), base_data, convert_ctx );
}

static void dl_internal_convert_collect_instances_from_ptr_array( dl_ctx_t            ctx,
																  const uint8_t*      array_data,
																  uintptr_t           array_count,
																  const dl_type_desc* sub_type,
																  const uint8_t*      base_data,
																  SConvertContext&    convert_ctx )
{
	uint32_t ptr_size = (uint32_t)dl_internal_ptr_size(convert_ctx.src_ptr_size);
	for( uintptr_t elem = 0; elem < array_count; ++elem )
		dl_internal_convert_collect_instances_from_ptr( ctx, sub_type, array_data + (elem * ptr_size), base_data, convert_ctx );
}

static dl_error_t dl_internal_convert_collect_instances_from_member( dl_ctx_t              ctx,
																	 const dl_member_desc* member,
																	 const uint8_t*        member_data,
																	 const uint8_t*        base_data,
																	 SConvertContext&      convert_ctx )
{
	dl_type_atom_t    atom_type    = member->AtomType();
	dl_type_storage_t storage_type = member->StorageType();

	switch(atom_type)
	{
		case DL_TYPE_ATOM_POD:
		{
			switch( storage_type )
			{
				case DL_TYPE_STORAGE_STR:
					dl_internal_convert_collect_instances_from_str( member_data, base_data, convert_ctx );
				break;
				case DL_TYPE_STORAGE_PTR:
					dl_internal_convert_collect_instances_from_ptr( ctx, dl_internal_find_type(ctx, member->type_id), member_data, base_data, convert_ctx );
				break;
				case DL_TYPE_STORAGE_STRUCT:
					dl_internal_convert_collect_instances(ctx, dl_internal_find_type(ctx, member->type_id), member_data, base_data, convert_ctx);
				break;
				default:
					break;
			}
		}
		break;
		case DL_TYPE_ATOM_INLINE_ARRAY:
		{
			switch(storage_type)
			{
				case DL_TYPE_STORAGE_STRUCT:
					dl_internal_convert_collect_instances_from_struct_array( ctx,
																			 member_data,
																			 member->inline_array_cnt(),
																			 dl_internal_find_type(ctx, member->type_id),
																			 base_data,
																			 convert_ctx );
					break;
				case DL_TYPE_STORAGE_STR:
					dl_internal_convert_collect_instances_from_str_array( member_data,
																		  member->inline_array_cnt(),
																		  base_data,
																		  convert_ctx );
					break;
				case DL_TYPE_STORAGE_PTR:
					dl_internal_convert_collect_instances_from_ptr_array( ctx,
																		  member_data,
																		  member->inline_array_cnt(),
																		  dl_internal_find_type(ctx, member->type_id),
																		  base_data,
																		  convert_ctx );
					break;
				default:
					DL_ASSERT(member->IsSimplePod());
					// ignore
			}
		}
		break;

		case DL_TYPE_ATOM_ARRAY:
		{
			uintptr_t offset = 0; uint32_t array_count = 0;
			dl_internal_read_array_data( member_data, &offset, &array_count, convert_ctx.src_endian, convert_ctx.src_ptr_size );

			if(offset == DL_NULL_PTR_OFFSET[convert_ctx.src_ptr_size])
			{
				DL_ASSERT( array_count == 0 );
				break;
			}

			const uint8_t* array_data = base_data + offset;
			const dl_type_desc* sub_type = 0x0;

			switch(storage_type)
			{
				case DL_TYPE_STORAGE_STR:
					dl_internal_convert_collect_instances_from_str_array(array_data, array_count, base_data, convert_ctx);
					break;
				case DL_TYPE_STORAGE_PTR:
					sub_type = dl_internal_find_type(ctx, member->type_id);
					dl_internal_convert_collect_instances_from_ptr_array( ctx,
																		  array_data,
																		  array_count,
																		  sub_type,
																		  base_data,
																		  convert_ctx );
					break;
				case DL_TYPE_STORAGE_STRUCT:
				{
					sub_type = dl_internal_find_type(ctx, member->type_id);
					dl_internal_convert_collect_instances_from_struct_array( ctx,
																			 array_data,
																			 array_count,
																			 sub_type,
																			 base_data,
																			 convert_ctx );
				}
				break;
				default:
					DL_ASSERT(member->IsSimplePod());
					break;
			}

			convert_ctx.instances.Add(SInstance(array_data, sub_type, array_count, member->type));
		}
		break;

		case DL_TYPE_ATOM_BITFIELD:
			// ignore
			break;

		default:
			DL_ASSERT(false && "Invalid ATOM-type!");
			break;
	}

	return DL_ERROR_OK;
}

static dl_error_t dl_internal_convert_collect_instances( dl_ctx_t            dl_ctx,
														 const dl_type_desc* type,
														 const uint8_t*      instance,
														 const uint8_t*      base_data,
														 SConvertContext&    convert_ctx )
{
	if( type->flags & DL_TYPE_FLAG_IS_UNION )
	{
		size_t type_offset = dl_internal_union_type_offset( dl_ctx, type, convert_ctx.src_ptr_size );

		// find member index from union type ...
		uint32_t union_type = *((uint32_t*)(instance + type_offset));
		const dl_member_desc* member = dl_internal_find_member_desc_by_name_hash( dl_ctx, type, union_type );
		const uint8_t* member_data = instance + member->offset[convert_ctx.src_ptr_size];

		return dl_internal_convert_collect_instances_from_member( dl_ctx, member, member_data, base_data, convert_ctx );
	}
	else
	{
		for( uint32_t member_index = 0; member_index < type->member_count; ++member_index )
		{
			const dl_member_desc* member = dl_get_type_member( dl_ctx, type, member_index );
			const uint8_t* member_data = instance + member->offset[convert_ctx.src_ptr_size];

			dl_error_t err = dl_internal_convert_collect_instances_from_member( dl_ctx, member, member_data, base_data, convert_ctx );
			if( err != DL_ERROR_OK )
				return err;
		}
	}

	return DL_ERROR_OK;
}

template<typename T>
static T dl_convert_bf_format( T old_val, const dl_member_desc* bf_members, uint32_t bf_members_count, SConvertContext* conv_ctx )
{
	T new_val = 0;

	for( uint32_t i = 0; i < bf_members_count; ++i )
	{
		const dl_member_desc& bf_member = bf_members[i];

		uint32_t bf_bits         = bf_member.bitfield_bits();
		uint32_t bf_offset       = bf_member.bitfield_offset();
		uint32_t bf_source_offset = dl_bf_offset( conv_ctx->src_endian, sizeof(T), bf_offset, bf_bits );
		uint32_t bf_target_offset = dl_bf_offset( conv_ctx->tgt_endian, sizeof(T), bf_offset, bf_bits );

 		T extracted = (T)DL_EXTRACT_BITS( old_val, T(bf_source_offset), T(bf_bits) );
 		new_val     = (T)DL_INSERT_BITS ( new_val, extracted, T(bf_target_offset), T(bf_bits) );
	}

	return new_val;
}

static uint8_t dl_convert_bit_field_format_uint8( uint8_t old_value, const dl_member_desc* first_bf_member, uint32_t num_bf_member, SConvertContext* conv_ctx )
{
	if( conv_ctx->src_endian != DL_ENDIAN_HOST )
		return dl_swap_endian_uint8( dl_convert_bf_format( dl_swap_endian_uint8( old_value ), first_bf_member, num_bf_member, conv_ctx ) );

	return dl_convert_bf_format( old_value, first_bf_member, num_bf_member, conv_ctx );
}

static uint16_t dl_convert_bit_field_format_uint16( uint16_t old_value, const dl_member_desc* first_bf_member, uint32_t num_bf_member, SConvertContext* conv_ctx )
{
	if( conv_ctx->src_endian != DL_ENDIAN_HOST )
		return dl_swap_endian_uint16( dl_convert_bf_format( dl_swap_endian_uint16( old_value ), first_bf_member, num_bf_member, conv_ctx ) );

	return dl_convert_bf_format( old_value, first_bf_member, num_bf_member, conv_ctx );
}

static uint32_t dl_convert_bit_field_format_uint32( uint32_t old_value, const dl_member_desc* first_bf_member, uint32_t num_bf_member, SConvertContext* conv_ctx )
{
	if( conv_ctx->src_endian != DL_ENDIAN_HOST )
		return dl_swap_endian_uint32( dl_convert_bf_format( dl_swap_endian_uint32( old_value ), first_bf_member, num_bf_member, conv_ctx ) );

	return dl_convert_bf_format( old_value, first_bf_member, num_bf_member, conv_ctx );
}

static uint64_t dl_convert_bit_field_format_uint64( uint64_t old_value, const dl_member_desc* first_bf_member, uint32_t num_bf_member, SConvertContext* conv_ctx )
{
	if( conv_ctx->src_endian != DL_ENDIAN_HOST )
		return dl_swap_endian_uint64( dl_convert_bf_format( dl_swap_endian_uint64( old_value ), first_bf_member, num_bf_member, conv_ctx ) );

	return dl_convert_bf_format( old_value, first_bf_member, num_bf_member, conv_ctx );
}

static void dl_internal_convert_save_patch_pos( SConvertContext* conv_ctx, dl_binary_writer* writer, size_t patch_pos, uintptr_t offset )
{
	if( offset == DL_NULL_PTR_OFFSET[conv_ctx->src_ptr_size] )
	{
		dl_binary_writer_write_ptr( writer, DL_NULL_PTR_OFFSET[conv_ctx->target_ptr_size] );
		return;
	}

	conv_ctx->m_lPatchOffset.Add( SConvertContext::PatchPos( patch_pos, offset ) );
	dl_binary_writer_write_ptr( writer, 0x0 );
}

static dl_error_t dl_internal_convert_write_struct( dl_ctx_t            dl_ctx,
													const uint8_t*      instance,
													const dl_type_desc* type,
													SConvertContext&    conv_ctx,
													dl_binary_writer*   writer );

static dl_error_t dl_internal_convert_write_member( dl_ctx_t              ctx,
													const uint8_t*        member_data,
													const dl_type_desc*   type,
													const dl_member_desc* member,
													uint32_t&             member_index, // TODO: ref == urgh!
													SConvertContext&      conv_ctx,
													dl_binary_writer*     writer )
{
	dl_type_atom_t    atom_type    = member->AtomType();
	dl_type_storage_t storage_type = member->StorageType();

	switch(atom_type)
	{
		case DL_TYPE_ATOM_POD:
		{
			switch(storage_type)
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					const dl_type_desc* sub_type = dl_internal_find_type(ctx, member->type_id);
					if(sub_type == 0x0)
						return DL_ERROR_TYPE_NOT_FOUND;
					dl_internal_convert_write_struct( ctx, member_data, sub_type, conv_ctx, writer );
				}
				break;
				case DL_TYPE_STORAGE_STR:
				case DL_TYPE_STORAGE_PTR:
				{
					uintptr_t offset = dl_internal_read_ptr_data(member_data, conv_ctx.src_endian, conv_ctx.src_ptr_size);
					dl_internal_convert_save_patch_pos( &conv_ctx, writer, dl_binary_writer_tell( writer), offset );
				}
				break;
				default:
					DL_ASSERT(member->IsSimplePod());
					dl_binary_writer_write_swap( writer, member_data, member->size[conv_ctx.src_ptr_size] );
					break;
			}
		}
		break;
		case DL_TYPE_ATOM_INLINE_ARRAY:
		{
			switch(storage_type)
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					const dl_type_desc* sub_type = dl_internal_find_type(ctx, member->type_id);
					if(sub_type == 0x0)
						return DL_ERROR_TYPE_NOT_FOUND;

					uintptr_t SubtypeSize = sub_type->size[conv_ctx.src_ptr_size];
					for( uint32_t i = 0; i < member->inline_array_cnt(); ++i )
						dl_internal_convert_write_struct( ctx, member_data + i * SubtypeSize, sub_type, conv_ctx, writer );
				}
				break;
				case DL_TYPE_STORAGE_STR:
				case DL_TYPE_STORAGE_PTR:
				{
					uintptr_t src_ptr_size = dl_internal_ptr_size(conv_ctx.src_ptr_size);
					uintptr_t tgt_ptr_size = dl_internal_ptr_size(conv_ctx.target_ptr_size);
					uintptr_t array_pos = dl_binary_writer_tell( writer );

					for (uintptr_t elem_index = 0; elem_index < member->inline_array_cnt(); ++elem_index)
					{
						uintptr_t offset = dl_internal_read_ptr_data(member_data + (elem_index * src_ptr_size), conv_ctx.src_endian, conv_ctx.src_ptr_size);
						dl_internal_convert_save_patch_pos( &conv_ctx, writer, array_pos + (elem_index * tgt_ptr_size), offset );
					}
				}
				break;
				default:
				{
					DL_ASSERT(member->IsSimplePod());
					dl_binary_writer_write_array( writer, member_data, member->inline_array_cnt(), dl_pod_size( member->StorageType() ) );
				}
				break;
			}
		}
		break;

		case DL_TYPE_ATOM_ARRAY:
		{
			uintptr_t offset = 0; uint32_t count = 0;
			dl_internal_read_array_data( member_data, &offset, &count, conv_ctx.src_endian, conv_ctx.src_ptr_size );
			dl_internal_convert_save_patch_pos( &conv_ctx, writer, dl_binary_writer_tell( writer ), offset );
			dl_binary_writer_write_4byte( writer, member_data + dl_internal_ptr_size( conv_ctx.src_ptr_size ) );
			if( conv_ctx.target_ptr_size == DL_PTR_SIZE_64BIT )
				dl_binary_writer_write_zero( writer, 4 );
		}
		break;

		case DL_TYPE_ATOM_BITFIELD:
		{
			uint32_t j = member_index;

			do { j++; } while(j < type->member_count && dl_get_type_member( ctx, type, j )->AtomType() == DL_TYPE_ATOM_BITFIELD);

			if(conv_ctx.src_endian != conv_ctx.tgt_endian)
			{
				uint32_t nBFMembers = j - member_index;

				switch(member->size[conv_ctx.src_ptr_size])
				{
					case 1:
					{
						uint8_t val = dl_convert_bit_field_format_uint8( *(uint8_t*)member_data, member, nBFMembers, &conv_ctx );
						dl_binary_writer_write_1byte( writer, &val );
					} break;
					case 2:
					{
						uint16_t val = dl_convert_bit_field_format_uint16( *(uint16_t*)member_data, member, nBFMembers, &conv_ctx );
						dl_binary_writer_write_2byte( writer, &val );
					} break;
					case 4:
					{
						uint32_t val = dl_convert_bit_field_format_uint32( *(uint32_t*)member_data, member, nBFMembers, &conv_ctx );
						dl_binary_writer_write_4byte( writer, &val );
					} break;
					case 8:
					{
						uint64_t val = dl_convert_bit_field_format_uint64( *(uint64_t*)member_data, member, nBFMembers, &conv_ctx );
						dl_binary_writer_write_8byte( writer, &val );
					} break;
					default:
						DL_ASSERT(false && "Not supported pod-size or bitfield-size!");
				}
			}
			else
				dl_binary_writer_write( writer, member_data, member->size[conv_ctx.src_ptr_size] );

			member_index = j - 1;
		}
		break;

		default:
			DL_ASSERT(false && "Invalid ATOM-type!");
			break;
	}

	return DL_ERROR_OK;
}

static dl_error_t dl_internal_convert_write_struct( dl_ctx_t            dl_ctx,
													const uint8_t*      instance,
													const dl_type_desc* type,
													SConvertContext&    conv_ctx,
													dl_binary_writer*   writer )
{
	dl_binary_writer_align( writer, type->alignment[conv_ctx.target_ptr_size] );
	uintptr_t pos = dl_binary_writer_tell( writer );
	dl_binary_writer_reserve( writer, type->size[conv_ctx.target_ptr_size] );

	if( type->flags & DL_TYPE_FLAG_IS_UNION )
	{
		// TODO: extract to helper-function?
		size_t src_type_offset = dl_internal_union_type_offset( dl_ctx, type, conv_ctx.src_ptr_size );
		size_t tgt_type_offset = dl_internal_union_type_offset( dl_ctx, type, conv_ctx.target_ptr_size );

		uint32_t union_type = *((uint32_t*)(instance + src_type_offset));
		const dl_member_desc* member = dl_internal_find_member_desc_by_name_hash( dl_ctx, type, union_type );
		const uint8_t* member_data = instance + member->offset[conv_ctx.src_ptr_size];

		uint32_t member_index = 0;
		dl_error_t err = dl_internal_convert_write_member( dl_ctx, member_data, type, member, member_index, conv_ctx, writer );
		if( err != DL_ERROR_OK )
			return err;

		dl_binary_writer_seek_set( writer, pos + tgt_type_offset );
		dl_binary_writer_align( writer, 4 );
		dl_binary_writer_write_uint32( writer, union_type );
	}
	else
	{
		for( uint32_t member_index = 0; member_index < type->member_count; ++member_index )
		{
			const dl_member_desc* member = dl_get_type_member( dl_ctx, type, member_index );
			const uint8_t* member_data   = instance + member->offset[conv_ctx.src_ptr_size];

			dl_binary_writer_align( writer, member->alignment[conv_ctx.target_ptr_size] );
			dl_error_t err = dl_internal_convert_write_member( dl_ctx, member_data, type, member, member_index, conv_ctx, writer );
			if( err != DL_ERROR_OK )
				return err;
		}
	}

	// we need to write our entire size with zeroes. Our entire size might be less than the sum of the members.
	uintptr_t pos_diff = dl_binary_writer_tell( writer ) - pos;

	if( pos_diff < type->size[conv_ctx.target_ptr_size] )
		dl_binary_writer_write_zero( writer, type->size[conv_ctx.target_ptr_size] - pos_diff );

	DL_ASSERT( dl_binary_writer_tell( writer ) - pos == type->size[conv_ctx.target_ptr_size] );

	return DL_ERROR_OK;
}

static dl_error_t dl_internal_convert_write_instance( dl_ctx_t          dl_ctx,
													  const SInstance&  inst,
													  uintptr_t*        new_offset,
													  SConvertContext&  conv_ctx,
													  dl_binary_writer* writer )
{
	union { const uint8_t* u8; const uint16_t* u16; const uint32_t* u32; const uint64_t* u64; const char* str; };
	u8 = inst.address;

	dl_binary_writer_seek_end( writer ); // place instance at the end!

	dl_type_atom_t    atom_type    = dl_type_atom_t((inst.type_id & DL_TYPE_ATOM_MASK) >> DL_TYPE_ATOM_MIN_BIT);
	dl_type_storage_t storage_type = dl_type_storage_t((inst.type_id & DL_TYPE_STORAGE_MASK) >> DL_TYPE_STORAGE_MIN_BIT);

	if(inst.type != 0x0)
		dl_binary_writer_align( writer, inst.type->alignment[conv_ctx.target_ptr_size] );
	else if(atom_type == DL_TYPE_ATOM_ARRAY)
	{
		switch(storage_type)
		{
			case DL_TYPE_STORAGE_STR:
			case DL_TYPE_STORAGE_PTR:
				dl_binary_writer_align( writer, conv_ctx.target_ptr_size == DL_PTR_SIZE_32BIT ? 4 : 8 );
				break;
			default:
				dl_binary_writer_align( writer, dl_pod_size( storage_type ) );
				break;
		}
	}

	*new_offset = dl_binary_writer_tell( writer );

	switch( atom_type )
	{
		case DL_TYPE_ATOM_ARRAY:
		{
			switch(storage_type)
			{
				case DL_TYPE_STORAGE_STRUCT:
				{
					uintptr_t type_size = inst.type->size[conv_ctx.src_ptr_size];
					for( uintptr_t elem = 0; elem < inst.array_count; ++elem )
					{
						dl_error_t err = dl_internal_convert_write_struct( dl_ctx, u8 + ( elem * type_size ), inst.type, conv_ctx, writer );
						if(err != DL_ERROR_OK) return err;
					}
				}
				break;

				case DL_TYPE_STORAGE_STR:
				{
					uintptr_t type_size = dl_internal_ptr_size(conv_ctx.src_ptr_size);
					for(uintptr_t elem = 0; elem < inst.array_count; ++elem )
					{
						uintptr_t offset = dl_internal_read_ptr_data(u8 + ( elem * type_size ), conv_ctx.src_endian, conv_ctx.src_ptr_size);
						dl_internal_convert_save_patch_pos( &conv_ctx, writer, dl_binary_writer_tell( writer ), offset );
					}
				}
				break;

				case DL_TYPE_STORAGE_PTR:
				{
					uintptr_t ptr_size = dl_internal_ptr_size(conv_ctx.src_ptr_size);
					for(uintptr_t elem = 0; elem < inst.array_count; ++elem )
					{
						uintptr_t offset = dl_internal_read_ptr_data(u8 + ( elem * ptr_size ), conv_ctx.src_endian, conv_ctx.src_ptr_size);
						dl_internal_convert_save_patch_pos( &conv_ctx, writer, dl_binary_writer_tell( writer ), offset );
					}
				}
				break;

				case DL_TYPE_STORAGE_INT8:
				case DL_TYPE_STORAGE_UINT8:
				case DL_TYPE_STORAGE_ENUM_INT8:
				case DL_TYPE_STORAGE_ENUM_UINT8:  dl_binary_writer_write_array( writer, u8, inst.array_count, sizeof(uint8_t) ); break;
				case DL_TYPE_STORAGE_INT16:
				case DL_TYPE_STORAGE_UINT16:
				case DL_TYPE_STORAGE_ENUM_INT16:
				case DL_TYPE_STORAGE_ENUM_UINT16: dl_binary_writer_write_array( writer, u16, inst.array_count, sizeof(uint16_t) ); break;
				case DL_TYPE_STORAGE_INT32:
				case DL_TYPE_STORAGE_UINT32:
				case DL_TYPE_STORAGE_FP32:
				case DL_TYPE_STORAGE_ENUM_INT32:
				case DL_TYPE_STORAGE_ENUM_UINT32: dl_binary_writer_write_array( writer, u32, inst.array_count, sizeof(uint32_t) ); break;
				case DL_TYPE_STORAGE_INT64:
				case DL_TYPE_STORAGE_UINT64:
				case DL_TYPE_STORAGE_FP64:
				case DL_TYPE_STORAGE_ENUM_INT64:
				case DL_TYPE_STORAGE_ENUM_UINT64: dl_binary_writer_write_array( writer, u64, inst.array_count, sizeof(uint64_t) ); break;

				default:
					DL_ASSERT(false && "Unknown storage type!");
					break;
			}

			return DL_ERROR_OK;
		}
		break;

		case DL_TYPE_ATOM_POD:
		{
			switch( storage_type )
			{
				case DL_TYPE_STORAGE_STR:
					dl_binary_writer_write_array( writer, u8,  strlen(str) + 1, sizeof(uint8_t) );
					return DL_ERROR_OK;
				case DL_TYPE_STORAGE_STRUCT:
				case DL_TYPE_STORAGE_PTR:
					return dl_internal_convert_write_struct( dl_ctx, u8, inst.type, conv_ctx, writer );
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

	return DL_ERROR_INTERNAL_ERROR;
}

#include <algorithm>

bool dl_internal_sort_pred( const SInstance& i1, const SInstance& i2 ) { return i1.address < i2.address; }

dl_error_t dl_internal_convert_no_header( dl_ctx_t       dl_ctx,
                                          unsigned char* packed_instance, unsigned char* packed_instance_base,
                                          unsigned char* out_instance,    size_t         out_instance_size,
                                          size_t*        needed_size,
                                          dl_endian_t    src_endian,      dl_endian_t    out_endian,
                                          dl_ptr_size_t  src_ptr_size,    dl_ptr_size_t  out_ptr_size,
                                          const dl_type_desc* root_type,  size_t         base_offset )
{
	dl_binary_writer writer;
	dl_binary_writer_init( &writer, out_instance, out_instance_size, out_instance == 0x0, src_endian, out_endian, out_ptr_size );

	SConvertContext conv_ctx( src_endian, out_endian, src_ptr_size, out_ptr_size );

	conv_ctx.instances.Add(SInstance(packed_instance, root_type, 0x0, dl_make_type(DL_TYPE_ATOM_POD, DL_TYPE_STORAGE_STRUCT)));
	dl_error_t err = dl_internal_convert_collect_instances(dl_ctx, root_type, packed_instance, packed_instance_base, conv_ctx);

	// TODO: we need to sort the instances here after their offset!

	SInstance* insts = conv_ctx.instances.GetBasePtr();
	std::sort( insts, insts + conv_ctx.instances.Len(), dl_internal_sort_pred );

	for(unsigned int i = 0; i < conv_ctx.instances.Len(); ++i)
	{
		err = dl_internal_convert_write_instance( dl_ctx, conv_ctx.instances[i], &conv_ctx.instances[i].offset_after_patch, conv_ctx, &writer );
		if(err != DL_ERROR_OK)
			return err;
	}

	if(out_instance != 0x0) // no need to patch data if we are only calculating size
	{
		for(unsigned int i = 0; i < conv_ctx.m_lPatchOffset.Len(); ++i)
		{
			SConvertContext::PatchPos& pp = conv_ctx.m_lPatchOffset[i];

			// find new offset
			uintptr_t new_offset = (uintptr_t)-1;

			for( size_t j = 0; j < conv_ctx.instances.Len(); ++j )
			{
				uintptr_t old_offset = (uintptr_t)(conv_ctx.instances[j].address - packed_instance_base);

				if(old_offset == pp.old_offset)
				{
					new_offset = conv_ctx.instances[j].offset_after_patch;
					break;
				}
			}

			DL_ASSERT(new_offset != (uintptr_t)-1 && "We should have found the instance!");

			dl_binary_writer_seek_set( &writer, pp.pos );
			dl_binary_writer_write_ptr( &writer, new_offset + base_offset );
		}
	}

	dl_binary_writer_seek_end( &writer );
	*needed_size = (unsigned int)dl_binary_writer_tell( &writer );

	return err;
}

static dl_error_t dl_internal_convert_instance( dl_ctx_t       dl_ctx,          dl_typeid_t type,
                                                unsigned char* packed_instance, size_t      packed_instance_size,
                                                unsigned char* out_instance,    size_t      out_instance_size,
                                                dl_endian_t    out_endian,      size_t      out_ptr_size,
                                                size_t*        out_size )
{
	dl_data_header* header = (dl_data_header*)packed_instance;

	if( packed_instance_size < sizeof(dl_data_header) )             return DL_ERROR_MALFORMED_DATA;
	if( header->id != DL_INSTANCE_ID &&
		header->id != DL_INSTANCE_ID_SWAPED )                       return DL_ERROR_MALFORMED_DATA;
	if( header->version != DL_INSTANCE_VERSION &&
		header->version != DL_INSTANCE_VERSION_SWAPED )             return DL_ERROR_VERSION_MISMATCH;
	if( header->root_instance_type != type &&
		header->root_instance_type != dl_swap_endian_uint32(type) ) return DL_ERROR_TYPE_MISMATCH;
	if( out_ptr_size != 4 && out_ptr_size != 8 )                    return DL_ERROR_INVALID_PARAMETER;

	dl_ptr_size_t src_ptr_size = header->is_64_bit_ptr != 0 ? DL_PTR_SIZE_64BIT : DL_PTR_SIZE_32BIT;
	dl_ptr_size_t dst_ptr_size;

	switch(out_ptr_size)
	{
		case 4: dst_ptr_size = DL_PTR_SIZE_32BIT; break;
		case 8: dst_ptr_size = DL_PTR_SIZE_64BIT; break;
		default: return DL_ERROR_INVALID_PARAMETER;
	}

	// converting to larger pointersize in an inplace conversion is not possible!
	if( dst_ptr_size > src_ptr_size && packed_instance == out_instance )
		return DL_ERROR_UNSUPPORTED_OPERATION;

	dl_endian_t src_endian = header->id == DL_INSTANCE_ID ? DL_ENDIAN_HOST : dl_other_endian( DL_ENDIAN_HOST );

	if(src_endian == out_endian && src_ptr_size == dst_ptr_size)
	{
		if(out_instance != 0x0)
			memmove(out_instance, packed_instance, packed_instance_size); // TODO: This is a bug! data_size is only the size of buffer, not the size of the packed instance!

		*out_size = packed_instance_size; // TODO: This is a bug! data_size is only the size of buffer, not the size of the packed instance!
		return DL_ERROR_OK;
	}

	dl_typeid_t root_type_id = src_endian != DL_ENDIAN_HOST ? dl_swap_endian_uint32( header->root_instance_type ) : header->root_instance_type;

	const dl_type_desc* root_type = dl_internal_find_type(dl_ctx, root_type_id);
	if(root_type == 0x0)
		return DL_ERROR_TYPE_NOT_FOUND;

	dl_error_t err = dl_internal_convert_no_header( dl_ctx,
												    packed_instance + sizeof(dl_data_header),
												    packed_instance + sizeof(dl_data_header),
												    out_instance == 0x0 ? 0x0 : out_instance + sizeof(dl_data_header),
												    out_instance_size - sizeof(dl_data_header),
												    out_size,
												    src_endian,
												    out_endian,
												    src_ptr_size,
												    dst_ptr_size,
												    root_type,
												    0u );

	if(out_instance != 0x0)
	{
		dl_data_header* new_header = (dl_data_header*)out_instance;
		new_header->id                 = DL_INSTANCE_ID;
		new_header->version            = DL_INSTANCE_VERSION;
		new_header->root_instance_type = type;
		new_header->instance_size      = uint32_t(*out_size);
		new_header->is_64_bit_ptr      = out_ptr_size == 4 ? 0 : 1;

		if(DL_ENDIAN_HOST != out_endian)
			dl_swap_header(new_header);
	}

	*out_size += sizeof(dl_data_header);
	return err;
}

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

dl_error_t dl_convert_inplace( dl_ctx_t       dl_ctx,          dl_typeid_t type,
		                       unsigned char* packed_instance, size_t      packed_instance_size,
		                       dl_endian_t    out_endian,      size_t      out_ptr_size,
		                       size_t*        produced_bytes )
{
	size_t dummy;
	if( produced_bytes == 0x0 )
		produced_bytes = &dummy;
	return dl_internal_convert_instance( dl_ctx, type, packed_instance, packed_instance_size, packed_instance, packed_instance_size, out_endian, out_ptr_size, produced_bytes );
}

dl_error_t dl_convert( dl_ctx_t       dl_ctx,          dl_typeid_t type,
                       unsigned char* packed_instance, size_t      packed_instance_size,
                       unsigned char* out_instance,    size_t      out_instance_size,
                       dl_endian_t    out_endian,      size_t      out_ptr_size,
                       size_t*        produced_bytes )
{
	DL_ASSERT(out_instance != packed_instance && "Src and destination can not be the same!");
	size_t dummy;
	if( produced_bytes == 0x0 )
		produced_bytes = &dummy;
	return dl_internal_convert_instance( dl_ctx, type, packed_instance, packed_instance_size, out_instance, out_instance_size, out_endian, out_ptr_size, produced_bytes );
}

dl_error_t dl_convert_calc_size( dl_ctx_t       dl_ctx,          dl_typeid_t type,
                                 unsigned char* packed_instance, size_t      packed_instance_size,
                                 size_t         out_ptr_size,    size_t*     out_size )
{
	return dl_convert( dl_ctx, type, packed_instance, packed_instance_size, 0x0, 0, DL_ENDIAN_HOST, out_ptr_size, out_size );
}

#ifdef __cplusplus
}
#endif  // __cplusplus
