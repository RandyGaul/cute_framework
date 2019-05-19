/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include "dl_txt_read.h"

#include <errno.h>
#include <stdlib.h>

long long dl_txt_pack_eat_strtoll( dl_ctx_t dl_ctx, dl_txt_read_ctx* read_ctx, long long range_min, long long range_max, const char* type )
{
	dl_txt_eat_white( read_ctx );

	long long v = dl_txt_eat_bool( read_ctx );
	if( v < 2 )
		return v;

	errno = 0;
	char* next = 0x0;
#if defined(_MSC_VER)
	v = _strtoi64( read_ctx->iter, &next, 0 );
#else
    v = strtoll( read_ctx->iter, &next, 0 );
#endif

	if( read_ctx->iter == next )
	{
		if( tolower(read_ctx->iter[0]) == 'm')
		{
			if( tolower(read_ctx->iter[1]) == 'a' &&
			    tolower(read_ctx->iter[2]) == 'x' &&
			   !isalnum(read_ctx->iter[3]) )
			{
				read_ctx->iter += 3;
				return range_max;
			}
			if( tolower(read_ctx->iter[1]) == 'i' &&
			    tolower(read_ctx->iter[2]) == 'n' &&
			   !isalnum(read_ctx->iter[3]))
			{
				read_ctx->iter += 3;
				return range_min;
			}
		}
		dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_MALFORMED_DATA, "expected a value of type '%s'", type );
	}
	if( !(v >= range_min && v <= range_max) || errno == ERANGE )
		dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_TXT_RANGE_ERROR, "expected a value of type '%s', %lld is out of range.", type, v );
	read_ctx->iter = next;
	return v;
}

unsigned long long dl_txt_pack_eat_strtoull( dl_ctx_t dl_ctx, dl_txt_read_ctx* read_ctx, unsigned long long range_max, const char* type )
{
	dl_txt_eat_white( read_ctx );

	if(read_ctx->iter[0] == '-')
		dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_TXT_RANGE_ERROR, "expected a value of unsigned type '%s', but value is negative!", type );

	unsigned long long v = (unsigned long long)dl_txt_eat_bool( read_ctx );
	if( v < 2 )
		return v;

	errno = 0;
	char* next = 0x0;
#if defined(_MSC_VER)
	v = _strtoui64( read_ctx->iter, &next, 0 );
#else
    v = strtoull( read_ctx->iter, &next, 0 );
#endif

	if( read_ctx->iter == next )
	{
		if( tolower(read_ctx->iter[0]) == 'm' )
		{
			if( tolower(read_ctx->iter[1]) == 'a' &&
			    tolower(read_ctx->iter[2]) == 'x' &&
			   !isalnum(read_ctx->iter[3]))
			{
				read_ctx->iter += 3;
				return range_max;
			}
			if( tolower(read_ctx->iter[1]) == 'i' &&
			    tolower(read_ctx->iter[2]) == 'n' &&
			   !isalnum(read_ctx->iter[3]))
			{
				read_ctx->iter += 3;
				return 0;
			}
		}
		dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_MALFORMED_DATA, "expected a value of type '%s'", type );
	}
	if( v > range_max || errno == ERANGE )
		dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_TXT_RANGE_ERROR, "expected a value of type '%s', %llu is out of range.", type, v );
	read_ctx->iter = next;
	return v;
}

void dl_report_error_location( dl_ctx_t ctx, const char* txt, const char* end, const char* error_pos )
{
	int line = 0;
	int col = 0;
	const char* last_line = txt;
	const char* iter = txt;
	while( iter != end && iter != error_pos )
	{
		if( *iter == '\n' )
		{
			last_line = iter + 1;
			++line;
			col = 0;
		}
		else
		{
			++col;
		}
		++iter;
	}

	if( iter == end )
		dl_log_error( ctx, "at end of buffer");
	else
	{
		const char* line_end = strchr( last_line, '\n' );
		dl_log_error( ctx, "at line %d, col %d:\n%.*s\n%*c^", line, col, (int)(line_end-last_line), last_line, col, ' ');
	}
}
