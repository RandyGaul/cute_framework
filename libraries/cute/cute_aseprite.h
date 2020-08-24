/*
	------------------------------------------------------------------------------
		Licensing information can be found at the end of the file.
	------------------------------------------------------------------------------

	cute_aseprite.h - v1.00

	To create implementation (the function definitions)
		#define CUTE_ASEPRITE_IMPLEMENTATION
	in *one* C/CPP file (translation unit) that includes this file


	SUMMARY

		cute_asesprite.h is a single-file header that implements some functions to
		parse .aseprite files. The entire file is parsed all at once and some
		structs are filled out then handed back to you.


	LIMITATIONS

		Only the "normal" blend mode for layers is supported. As a workaround try
		using the "merge down" function in Aseprite to create a normal layer.
		Supporting all blend modes would take too much code to be worth it.


	Revision history:
		1.00 (00/00/0000) initial release
/*

/*
	DOCUMENTATION
*/

#ifndef CUTE_ASEPRITE_H
#define CUTE_ASEPRITE_H

typedef struct cute_aseprite_t cute_aseprite_t;

cute_aseprite_t* cute_aseprite_load_from_file(const char* path, void* mem_ctx);
cute_aseprite_t* cute_aseprite_load_from_memory(const void* memory, int size, void* mem_ctx);
void cute_aseprite_free(cute_aseprite_t* aseprite);

#endif // CUTE_ASEPRITE_H

#ifdef CUTE_ASEPRITE_IMPLEMENTATION
#ifndef CUTE_ASEPRITE_IMPLEMENTATION_ONCE
#define CUTE_ASEPRITE_IMPLEMENTATION_ONCE

#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
	#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#if !defined(CUTE_ASEPRITE_ALLOC)
	#include <stdlib.h>
	#define CUTE_ASEPRITE_ALLOC(size, ctx) malloc(size)
	#define CUTE_ASEPRITE_FREE(mem, ctx) free(mem)
#endif

#if !defined(CUTE_ASEPRITE_UNUSED)
	#if defined(_MSC_VER)
		#define CUTE_ASEPRITE_UNUSED(x) (void)x
	#else
		#define CUTE_ASEPRITE_UNUSED(x) (void)(sizeof(x))
	#endif
#endif

#if !defined(CUTE_ASEPRITE_MEMCPY)
	#include <string.h> // memcpy
	#define CUTE_ASEPRITE_MEMCPY memcpy
#endif

#if !defined(CUTE_ASEPRITE_MEMSET)
	#include <string.h> // memset
	#define CUTE_ASEPRITE_MEMSET memset
#endif

#if !defined(CUTE_ASEPRITE_ASSERT)
	#include <assert.h>
	#define CUTE_ASEPRITE_ASSERT assert
#endif

static int s_error_cline;               // The line in cute_aseprite.h where the error was triggered.
static const char* s_error_file = NULL; // The filepath of the file being parsed. NULL if from memory.
static const char* s_error_reason;      // Used to capture errors during DEFLATE parsing.

#if !defined(CUTE_ASEPRITE_WARNING)
	#define CUTE_ASEPRITE_WARNING(msg) cute_aseprite_warning(msg, __LINE__)

	void cute_aseprite_warning(const char* warning, int line)
	{
		s_error_cline = line;
		const char *error_file = s_error_file ? s_error_file : "MEMORY";
		printf("WARNING (cute_tiled.h:%i): %s (%s)\n", s_error_cline, warning, error_file);
	}
#endif

#include <stdint.h>

#define CUTE_ASEPRITE_FAIL() do { goto ase_err; } while (0)
#define CUTE_ASEPRITE_CHECK(X, Y) do { if (!(X)) { s_error_reason = Y; CUTE_ASEPRITE_FAIL(); } } while (0)
#define CUTE_ASEPRITE_CALL(X) do { if (!(X)) goto ase_err; } while (0)
#define CUTE_ASEPRITE_LOOKUP_BITS 9
#define CUTE_ASEPRITE_LOOKUP_COUNT (1 << CUTE_ASEPRITE_LOOKUP_BITS)
#define CUTE_ASEPRITE_LOOKUP_MASK (CUTE_ASEPRITE_LOOKUP_COUNT - 1)
#define CUTE_ASEPRITE_DEFLATE_MAX_BITLEN 15

// DEFLATE tables from RFC 1951
static uint8_t s_fixed_table[288 + 32] = {
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
}; // 3.2.6
static uint8_t s_permutation_order[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 }; // 3.2.7
static uint8_t s_len_extra_bits[29 + 2] = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,  0,0 }; // 3.2.5
static uint32_t s_len_base[29 + 2] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258,  0,0 }; // 3.2.5
static uint8_t s_dist_extra_bits[30 + 2] = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,  0,0 }; // 3.2.5
static uint32_t s_dist_base[30 + 2] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577, 0,0 }; // 3.2.5

typedef struct ase_state_t
{
	uint64_t bits;
	int count;
	uint32_t* words;
	int word_count;
	int word_index;
	int bits_left;

	int final_word_available;
	uint32_t final_word;

	char* out;
	char* out_end;
	char* begin;

	uint16_t lookup[CUTE_ASEPRITE_LOOKUP_COUNT];
	uint32_t lit[288];
	uint32_t dst[32];
	uint32_t len[19];
	uint32_t nlit;
	uint32_t ndst;
	uint32_t nlen;
} ase_state_t;

static int s_would_overflow(ase_state_t* s, int num_bits)
{
	return (s->bits_left + s->count) - num_bits < 0;
}

static char* s_ptr(ase_state_t* s)
{
	CUTE_ASEPRITE_ASSERT(!(s->bits_left & 7));
	return (char*)(s->words + s->word_index) - (s->count / 8);
}

static uint64_t s_peak_bits(ase_state_t* s, int num_bits_to_read)
{
	if (s->count < num_bits_to_read)
	{
		if (s->word_index < s->word_count)
		{
			uint32_t word = s->words[s->word_index++];
			s->bits |= (uint64_t)word << s->count;
			s->count += 32;
			CUTE_ASEPRITE_ASSERT(s->word_index <= s->word_count);
		}

		else if (s->final_word_available)
		{
			uint32_t word = s->final_word;
			s->bits |= (uint64_t)word << s->count;
			s->count += s->bits_left;
			s->final_word_available = 0;
		}
	}

	return s->bits;
}

static uint32_t s_consume_bits(ase_state_t* s, int num_bits_to_read)
{
	CUTE_ASEPRITE_ASSERT(s->count >= num_bits_to_read);
	uint32_t bits = s->bits & (((uint64_t)1 << num_bits_to_read) - 1);
	s->bits >>= num_bits_to_read;
	s->count -= num_bits_to_read;
	s->bits_left -= num_bits_to_read;
	return bits;
}

static uint32_t s_read_bits(ase_state_t* s, int num_bits_to_read)
{
	CUTE_ASEPRITE_ASSERT(num_bits_to_read <= 32);
	CUTE_ASEPRITE_ASSERT(num_bits_to_read >= 0);
	CUTE_ASEPRITE_ASSERT(s->bits_left > 0);
	CUTE_ASEPRITE_ASSERT(s->count <= 64);
	CUTE_ASEPRITE_ASSERT(!s_would_overflow(s, num_bits_to_read));
	s_peak_bits(s, num_bits_to_read);
	uint32_t bits = s_consume_bits(s, num_bits_to_read);
	return bits;
}

static uint32_t s_rev16(uint32_t a)
{
	a = ((a & 0xAAAA) >>  1) | ((a & 0x5555) << 1);
	a = ((a & 0xCCCC) >>  2) | ((a & 0x3333) << 2);
	a = ((a & 0xF0F0) >>  4) | ((a & 0x0F0F) << 4);
	a = ((a & 0xFF00) >>  8) | ((a & 0x00FF) << 8);
	return a;
}

// RFC 1951 section 3.2.2
static int s_build(ase_state_t* s, uint32_t* tree, uint8_t* lens, int sym_count)
{
	int n, codes[16], first[16], counts[16] = { 0 };

	// Frequency count
	for (n = 0; n < sym_count; n++) counts[lens[n]]++;

	// Distribute codes
	counts[0] = codes[0] = first[0] = 0;
	for (n = 1; n <= 15; ++n)
	{
		codes[n] = (codes[n - 1] + counts[n - 1]) << 1;
		first[n] = first[n - 1] + counts[n - 1];
	}

	if (s) CUTE_ASEPRITE_MEMSET(s->lookup, 0, sizeof(s->lookup));
	for (int i = 0; i < sym_count; ++i)
	{
		int len = lens[i];

		if (len != 0)
		{
			CUTE_ASEPRITE_ASSERT(len < 16);
			uint32_t code = codes[len]++;
			uint32_t slot = first[len]++;
			tree[slot] = (code << (32 - len)) | (i << 4) | len;

			if (s && len <= CUTE_ASEPRITE_LOOKUP_BITS)
			{
				int j = s_rev16(code) >> (16 - len);
				while (j < (1 << CUTE_ASEPRITE_LOOKUP_BITS))
				{
					s->lookup[j] = (uint16_t)((len << CUTE_ASEPRITE_LOOKUP_BITS) | i);
					j += (1 << len);
				}
			}
		}
	}

	int max_index = first[15];
	return max_index;
}

static int s_stored(ase_state_t* s)
{
	char* p;

	// 3.2.3
	// skip any remaining bits in current partially processed byte
	s_read_bits(s, s->count & 7);

	// 3.2.4
	// read LEN and NLEN, should complement each other
	uint16_t LEN = (uint16_t)s_read_bits(s, 16);
	uint16_t NLEN = (uint16_t)s_read_bits(s, 16);
	CUTE_ASEPRITE_CHECK(LEN == (uint16_t)(~NLEN), "Failed to find LEN and NLEN as complements within stored (uncompressed) stream.");
	CUTE_ASEPRITE_CHECK(s->bits_left / 8 <= (int)LEN, "Stored block extends beyond end of input stream.");
	p = s_ptr(s);
	CUTE_ASEPRITE_MEMCPY(s->out, p, LEN);
	s->out += LEN;
	return 1;

ase_err:
	return 0;
}

// 3.2.6
static int s_fixed(ase_state_t* s)
{
	s->nlit = s_build(s, s->lit, s_fixed_table, 288);
	s->ndst = s_build(0, s->dst, s_fixed_table + 288, 32);
	return 1;
}

static int s_decode(ase_state_t* s, uint32_t* tree, int hi)
{
	uint64_t bits = s_peak_bits(s, 16);
	uint32_t search = (s_rev16((uint32_t)bits) << 16) | 0xFFFF;
	int lo = 0;
	while (lo < hi)
	{
		int guess = (lo + hi) >> 1;
		if (search < tree[guess]) hi = guess;
		else lo = guess + 1;
	}

	uint32_t key = tree[lo - 1];
	uint32_t len = (32 - (key & 0xF));
	CUTE_ASEPRITE_ASSERT((search >> len) == (key >> len));

	int code = s_consume_bits(s, key & 0xF);
	(void)code;
	return (key >> 4) & 0xFFF;
}

// 3.2.7
static int s_dynamic(ase_state_t* s)
{
	uint8_t lenlens[19] = { 0 };

	int nlit = 257 + s_read_bits(s, 5);
	int ndst = 1 + s_read_bits(s, 5);
	int nlen = 4 + s_read_bits(s, 4);

	for (int i = 0 ; i < nlen; ++i)
		lenlens[s_permutation_order[i]] = (uint8_t)s_read_bits(s, 3);

	// Build the tree for decoding code lengths
	s->nlen = s_build(0, s->len, lenlens, 19);
	uint8_t lens[288 + 32];

	for (int n = 0; n < nlit + ndst;)
	{
		int sym = s_decode(s, s->len, s->nlen);
		switch (sym)
		{
		case 16: for (int i =  3 + s_read_bits(s, 2); i; --i, ++n) lens[n] = lens[n - 1]; break;
		case 17: for (int i =  3 + s_read_bits(s, 3); i; --i, ++n) lens[n] = 0; break;
		case 18: for (int i = 11 + s_read_bits(s, 7); i; --i, ++n) lens[n] = 0; break;
		default: lens[n++] = (uint8_t)sym; break;
		}
	}

	s->nlit = s_build(s, s->lit, lens, nlit);
	s->ndst = s_build(0, s->dst, lens + nlit, ndst);
	return 1;
}

// 3.2.3
static int s_block(ase_state_t* s)
{
	while (1)
	{
		int symbol = s_decode(s, s->lit, s->nlit);

		if (symbol < 256)
		{
			CUTE_ASEPRITE_CHECK(s->out + 1 <= s->out_end, "Attempted to overwrite out buffer while outputting a symbol.");
			*s->out = (char)symbol;
			s->out += 1;
		}

		else if (symbol > 256)
		{
			symbol -= 257;
			int length = s_read_bits(s, s_len_extra_bits[symbol]) + s_len_base[symbol];
			int distance_symbol = s_decode(s, s->dst, s->ndst);
			int backwards_distance = s_read_bits(s, s_dist_extra_bits[distance_symbol]) + s_dist_base[distance_symbol];
			CUTE_ASEPRITE_CHECK(s->out - backwards_distance >= s->begin, "Attempted to write before out buffer (invalid backwards distance).");
			CUTE_ASEPRITE_CHECK(s->out + length <= s->out_end, "Attempted to overwrite out buffer while outputting a string.");
			char* src = s->out - backwards_distance;
			char* dst = s->out;
			s->out += length;

			switch (backwards_distance)
			{
			case 1: // very common in images
				CUTE_ASEPRITE_MEMSET(dst, *src, length);
				break;
			default: while (length--) *dst++ = *src++;
			}
		}

		else break;
	}

	return 1;

ase_err:
	return 0;
}
	
// 3.2.3
static int s_inflate(void* in, int in_bytes, void* out, int out_bytes, void* mem_ctx)
{
	ase_state_t* s = (ase_state_t*)CUTE_ASEPRITE_ALLOC(sizeof(ase_state_t), mem_ctx);
	s->bits = 0;
	s->count = 0;
	s->word_index = 0;
	s->bits_left = in_bytes * 8;

	// s->words is the in-pointer rounded up to a multiple of 4
	int first_bytes = (int) ((( (size_t) in + 3) & ~3) - (size_t) in);
	s->words = (uint32_t*)((char*)in + first_bytes);
	s->word_count = (in_bytes - first_bytes) / 4;
	int last_bytes = ((in_bytes - first_bytes) & 3);

	for (int i = 0; i < first_bytes; ++i)
		s->bits |= (uint64_t)(((uint8_t*)in)[i]) << (i * 8);

	s->final_word_available = last_bytes ? 1 : 0;
	s->final_word = 0;
	for(int i = 0; i < last_bytes; i++) 
		s->final_word |= ((uint8_t*)in)[in_bytes - last_bytes+i] << (i * 8);

	s->count = first_bytes * 8;

	s->out = (char*)out;
	s->out_end = s->out + out_bytes;
	s->begin = (char*)out;

	int count = 0;
	int bfinal;
	do
	{
		bfinal = s_read_bits(s, 1);
		int btype = s_read_bits(s, 2);

		switch (btype)
		{
		case 0: CUTE_ASEPRITE_CALL(s_stored(s)); break;
		case 1: s_fixed(s); CUTE_ASEPRITE_CALL(s_block(s)); break;
		case 2: s_dynamic(s); CUTE_ASEPRITE_CALL(s_block(s)); break;
		case 3: CUTE_ASEPRITE_CHECK(0, "Detected unknown block type within input stream.");
		}

		++count;
	}
	while (!bfinal);

	CUTE_ASEPRITE_FREE(s, mem_ctx);
	return 1;

ase_err:
	CUTE_ASEPRITE_FREE(s, mem_ctx);
	return 0;
}

typedef struct ase_t
{
	uint8_t* in;
	uint8_t* end;
	void* mem_ctx;
} ase_t;

typedef struct ase_fixed_t
{
	uint16_t a;
	uint16_t b;
} ase_fixed_t;

static uint8_t s_read_uint8(ase_t* ase)
{
	CUTE_ASEPRITE_ASSERT(ase->in <= ase->end + sizeof(uint8_t));
	uint8_t** p = &ase->in;
	uint8_t value = **p;
	++(*p);
	return value;
}

static uint16_t s_read_uint16(ase_t* ase)
{
	CUTE_ASEPRITE_ASSERT(ase->in <= ase->end + sizeof(uint16_t));
	uint8_t** p = &ase->in;
	uint16_t value;
	value = (*p)[0];
	value |= (((uint16_t)((*p)[1])) << 8);
	*p += 2;
	return value;
}

static ase_fixed_t s_read_fixed(ase_t* ase)
{
	ase_fixed_t value;
	value.a = s_read_uint16(ase);
	value.b = s_read_uint16(ase);
	return value;
}

static uint32_t s_read_uint32(ase_t* ase)
{
	CUTE_ASEPRITE_ASSERT(ase->in <= ase->end + sizeof(uint32_t));
	uint8_t** p = &ase->in;
	uint32_t value;
	value  = (*p)[0];
	value |= (((uint32_t)((*p)[1])) << 8);
	value |= (((uint32_t)((*p)[2])) << 16);
	value |= (((uint32_t)((*p)[3])) << 24);
	*p += 4;
	return value;
}

static uint64_t s_read_uint64(ase_t* ase)
{
	CUTE_ASEPRITE_ASSERT(ase->in <= ase->end + sizeof(uint64_t));
	uint8_t** p = &ase->in;
	uint64_t value;
	value  = (*p)[0];
	value |= (((uint64_t)((*p)[1])) << 8 );
	value |= (((uint64_t)((*p)[2])) << 16);
	value |= (((uint64_t)((*p)[3])) << 24);
	value |= (((uint64_t)((*p)[4])) << 32);
	value |= (((uint64_t)((*p)[5])) << 40);
	value |= (((uint64_t)((*p)[6])) << 48);
	value |= (((uint64_t)((*p)[7])) << 56);
	*p += 8;
	return value;
}

static int16_t s_read_int16(ase_t* ase) { return (int16_t)s_read_uint16(ase); }
static int16_t s_read_int32(ase_t* ase) { return (int32_t)s_read_uint32(ase); }

static void s_read_bytes(ase_t* ase, uint8_t* bytes, int num_bytes)
{
	for (int i = 0; i < num_bytes; ++i) {
		bytes[i] = s_read_uint8(ase);
	}
}

static const char* s_read_string(ase_t* ase)
{
	int len = (int)s_read_uint16(ase);
	char* bytes = (char*)CUTE_ALLOC(len + 1, ase->mem_ctx);
	for (int i = 0; i < len; ++i) {
		bytes[i] = s_read_uint8(ase);
	}
	bytes[len] = 0;
	return bytes;
}

static void s_skip(ase_t* ase, int num_bytes)
{
	CUTE_ASEPRITE_ASSERT(ase->in <= ase->end + num_bytes);
	ase->in += num_bytes;
}

static char* s_fopen(const char* path, int* size, void* mem_ctx)
{
	CUTE_ASEPRITE_UNUSED(mem_ctx);
	char* data = 0;
	FILE* fp = fopen(path, "rb");
	int sz = 0;

	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		sz = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		data = (char*)CUTE_ASEPRITE_ALLOC(sz + 1, mem_ctx);
		fread(data, sz, 1, fp);
		data[sz] = 0;
		fclose(fp);
	}

	if (size) *size = sz;
	return data;
}

static ase_t* s_ase(const void* memory, int size, void* mem_ctx)
{
	ase_t* ase = (ase_t*)CUTE_ASEPRITE_ALLOC(sizeof(ase_t), mem_ctx);
	CUTE_ASEPRITE_MEMSET(ase, 0, sizeof(ase_t));
	ase->in = (uint8_t*)memory;
	ase->end = ase->in + size;
	ase->mem_ctx = mem_ctx;
	return ase;
}

cute_aseprite_t* cute_aseprite_load_from_file(const char* path, void* mem_ctx)
{
	s_error_file = path;
	int sz;
	void* file = s_fopen(path, &sz, mem_ctx);
	if (!file) {
		CUTE_ASEPRITE_WARNING("Unable to find map file.");
		return NULL;
	}
	cute_aseprite_t* aseprite = cute_aseprite_load_from_memory(file, sz, mem_ctx);
	CUTE_ASEPRITE_FREE(file, mem_ctx);
	s_error_file = NULL;
	return aseprite;
}

typedef struct ase_header_t
{
	uint16_t frames;
	uint16_t w, h;
	uint16_t color_depth;
	int32_t flags;
	int16_t speed;
	uint8_t transparent_index;
	uint16_t number_of_colors;
	uint8_t pixel_w;
	uint8_t pixel_h;
	int16_t grid_x;
	int16_t grid_y;
	uint16_t grid_w;
	uint16_t grid_h;
} ase_header_t;

typedef struct ase_frame_header_t
{
	uint16_t old_number_of_chunks;
	uint16_t duration_milliseconds;
	uint32_t number_of_chunks;
} ase_frame_header_t;

typedef struct ase_layer_chunk_t
{
	uint16_t flags;
	uint16_t layer_type;
	uint16_t layer_child_level;
	uint16_t blend_mode;
	uint8_t opacity;
	const char* layer_name;
} ase_layer_chunk_t;

typedef struct ase_cel_chunk_t
{
	uint16_t layer_index;
	int16_t x, y;
	uint8_t opacity;
	int16_t cel_type;

	union
	{
		struct {
			uint16_t w, h;
			void* pixels;
		} raw_cel;
		struct {
			uint16_t frame_index;
		} linked_cel;
		struct {
			uint16_t w, h;
			void* pixels;
		} compressed_image;
	} u;
} ase_cel_chunk_t;

typedef struct ase_cel_extra_chunk_t
{
	uint32_t flags;
	ase_fixed_t precise_x;
	ase_fixed_t precise_y;
	ase_fixed_t w, h;
} ase_cel_extra_chunk_t;

typedef struct ase_color_profile_chunk_t
{
	uint16_t type;
	uint16_t flags;
	ase_fixed_t gamma;
	uint32_t icc_profile_data_length;
	void* icc_profile_data;
} ase_color_profile_chunk_t;

typedef struct ase_tag_t
{
	uint16_t from_frame;
	uint16_t to_frame;
	uint8_t loop_animation_direction;
	uint8_t r, g, b;
	const char* name;
} ase_tag_t;

typedef struct ase_tags_chunk_t
{
	uint16_t tag_count;
	ase_tag_t* tags;
} ase_tags_chunk_t;

typedef struct ase_palette_entry_t
{
	uint8_t r, g, b, a;
	const char* color_name;
} ase_palette_entry_t;

typedef struct ase_palette_t
{
	uint32_t entry_count;
	ase_palette_entry_t* entries;
} ase_palette_t;

typedef struct ase_slice_t
{
	uint32_t frame_number;
	int16_t origin_x;
	int16_t origin_y;
	uint32_t w, h;
	int16_t center_x;
	int16_t center_y;
	uint32_t center_w;
	uint32_t center_h;
	int16_t pivot_x;
	int16_t pivot_y;
} ase_slice_t;

typedef struct ase_slice_chunk_t
{
	uint32_t slice_count;
	uint32_t flags;
	const char* name;
	ase_slice_t* slices;
} ase_slice_chunk_t;

cute_aseprite_t* cute_aseprite_load_from_memory(const void* memory, int size, void* mem_ctx)
{
	ase_t* ase = s_ase(memory, size, mem_ctx);

	s_skip(ase, sizeof(uint32_t)); // File size.
	CUTE_ASSERT(s_read_uint16(ase) == 0xA5E0);

	ase_header_t header;
	header.frames = s_read_uint16(ase);
	header.w = s_read_uint16(ase);
	header.h = s_read_uint16(ase);
	header.color_depth = s_read_uint16(ase);
	header.flags = s_read_uint32(ase);
	header.speed = s_read_uint16(ase);
	s_skip(ase, sizeof(uint32_t) * 2); // Spec says skip these bytes.
	header.transparent_index = s_read_uint8(ase);
	s_skip(ase, 3); // Spec says skip these bytes.
	header.number_of_colors = s_read_uint16(ase);
	header.pixel_w = s_read_uint8(ase);
	header.pixel_h = s_read_uint8(ase);
	header.grid_x = s_read_int16(ase);
	header.grid_y = s_read_int16(ase);
	header.grid_w = s_read_uint16(ase);
	header.grid_h = s_read_uint16(ase);
	s_skip(ase, 84); // For future use (set to zero).

	ase_palette_t palette;
	palette.entry_count = 0;
	palette.entries = NULL;

	for (int i = 0; i < header.frames; ++i) {
		ase_frame_header_t frame_header;
		s_skip(ase, sizeof(uint32_t)); // Frame size.
		CUTE_ASSERT(s_read_uint16(ase) == 0xF1FA);
		frame_header.old_number_of_chunks = s_read_uint16(ase);
		frame_header.duration_milliseconds = s_read_uint16(ase);
		s_skip(ase, 2); // For future use (set to zero).
		frame_header.number_of_chunks = s_read_uint32(ase);
		if (!frame_header.number_of_chunks) frame_header.number_of_chunks = frame_header.old_number_of_chunks;

		for (int j = 0; j < (int)frame_header.number_of_chunks; ++j) {
			uint32_t chunk_size = s_read_uint32(ase);
			uint16_t chunk_type = s_read_uint16(ase);
			chunk_size -= sizeof(uint32_t) + sizeof(uint16_t);
			uint8_t* chunk_start = ase->in;

			switch (chunk_type) {
			case 0x2004: // Layer chunk.
			{
				ase_layer_chunk_t chunk;
				chunk.flags = s_read_uint16(ase);
				chunk.layer_type = s_read_uint16(ase);
				chunk.layer_child_level = s_read_uint16(ase);
				s_skip(ase, sizeof(uint16_t)); // Default layer width in pixels (ignored).
				s_skip(ase, sizeof(uint16_t)); // Default layer height in pixels (ignored).
				chunk.blend_mode = s_read_uint16(ase);
				chunk.opacity = s_read_uint8(ase);
				s_skip(ase, 3); // For future use (set to zero).
				chunk.layer_name = s_read_string(ase);
			}	break;

			case 0x2005: // Cel chunk.
			{
				ase_cel_chunk_t chunk;
				chunk.layer_index = s_read_uint16(ase);
				chunk.x = s_read_int16(ase);
				chunk.y = s_read_int16(ase);
				chunk.opacity = s_read_uint8(ase);
				chunk.cel_type = s_read_uint16(ase);
				s_skip(ase, 7); // For future (set to zero).
				switch (chunk.cel_type) {
				case 0:
					chunk.u.raw_cel.w = s_read_uint16(ase);
					chunk.u.raw_cel.h = s_read_uint16(ase);
					chunk.u.raw_cel.pixels = ase->in;
					s_skip(ase, chunk.u.raw_cel.w * chunk.u.raw_cel.h * header.color_depth);
					break;

				case 1:
					chunk.u.linked_cel.frame_index = s_read_uint16(ase);
					break;

				case 2:
				{
					chunk.u.raw_cel.w = s_read_uint16(ase);
					chunk.u.raw_cel.h = s_read_uint16(ase);
					int zlib_byte0 = s_read_uint8(ase);
					int zlib_byte1 = s_read_uint8(ase);
					int deflate_bytes = chunk_size - (int)(ase->in - chunk_start);
					chunk.u.raw_cel.pixels = ase->in;
					CUTE_ASEPRITE_ASSERT((zlib_byte0 & 0x0F) == 0x08); // Only zlib compression method (RFC 1950) is supported.
					CUTE_ASEPRITE_ASSERT((zlib_byte0 & 0xF0) <= 0x70); // Innapropriate window size detected.
					CUTE_ASEPRITE_ASSERT(!(zlib_byte1 & 0x20)); // Preset dictionary is present and not supported.
					int pixels_sz = chunk.u.raw_cel.w * chunk.u.raw_cel.h * header.color_depth / 8;
					void* pixels = CUTE_ALLOC(pixels_sz, mem_ctx);
					int ret = s_inflate(chunk.u.raw_cel.pixels, deflate_bytes, pixels, pixels_sz, mem_ctx);
					if (!ret) CUTE_ASEPRITE_WARNING(s_error_reason);
					chunk.u.raw_cel.pixels = pixels;
					s_skip(ase, deflate_bytes);
				}	break;
				}
			}	break;

			case 0x2006: // Cel extra chunk.
			{
				ase_cel_extra_chunk_t chunk;
				chunk.flags = s_read_uint32(ase);
				chunk.precise_x = s_read_fixed(ase);
				chunk.precise_y = s_read_fixed(ase);
				chunk.w = s_read_fixed(ase);
				chunk.h = s_read_fixed(ase);
				s_skip(ase, 16); // For future use (set to zero).
			}	break;

			case 0x2007: // Color profile chunk.
			{
				ase_color_profile_chunk_t chunk;
				chunk.type = s_read_uint16(ase);
				chunk.flags = s_read_uint16(ase);
				chunk.gamma = s_read_fixed(ase);
				s_skip(ase, 8); // For future use (set to zero).
				if (chunk.type == 2) {
					// Use the embedded ICC profile.
					chunk.icc_profile_data_length = s_read_uint32(ase);
					chunk.icc_profile_data = CUTE_ALLOC(chunk.icc_profile_data_length, mem_ctx);
					CUTE_ASEPRITE_MEMCPY(chunk.icc_profile_data, ase->in, chunk.icc_profile_data_length);
				} else {
					chunk.icc_profile_data_length = 0;
					chunk.icc_profile_data = NULL;
				}
			}	break;

			case 0x2018: // Tags chunk.
			{
				ase_tags_chunk_t chunk;
				chunk.tag_count = s_read_uint16(ase);
				chunk.tags = (ase_tag_t*)CUTE_ASEPRITE_ALLOC(sizeof(ase_tag_t) * chunk.tag_count, mem_ctx);
				for (int k = 0; k < chunk.tag_count; ++k) {
					ase_tag_t tag;
					tag.from_frame = s_read_uint16(ase);
					tag.to_frame = s_read_uint16(ase);
					tag.loop_animation_direction = s_read_uint8(ase);
					s_skip(ase, 8); // For future (set to zero).
					tag.r = s_read_uint8(ase);
					tag.g = s_read_uint8(ase);
					tag.b = s_read_uint8(ase);
					s_skip(ase, 8); // Extra byte (zero).
					tag.name = s_read_string(ase);
					chunk.tags[k] = tag;
				}
			}	break;

			case 0x2019: // Palette chunk.
			{
				palette.entry_count = s_read_uint32(ase);
				palette.entries = (ase_palette_entry_t*)CUTE_ASEPRITE_ALLOC(sizeof(ase_palette_entry_t) * palette.entry_count, mem_ctx);
				CUTE_ASEPRITE_MEMSET(palette.entries, 0, sizeof(ase_palette_entry_t) * palette.entry_count);
				int first_index = s_read_uint32(ase);
				int last_index = s_read_uint32(ase);
				s_skip(ase, 8); // For future (set to zero).
				for (int k = first_index; k <= last_index; ++k) {
					int has_name = s_read_uint16(ase);
					ase_palette_entry_t entry;
					entry.r = s_read_uint8(ase);
					entry.g = s_read_uint8(ase);
					entry.b = s_read_uint8(ase);
					entry.a = s_read_uint8(ase);
					if (has_name) {
						entry.color_name = s_read_string(ase);
					} else {
						entry.color_name = NULL;
					}
					palette.entries[k] = entry;
				}
			}	break;

			case 0x2022: // Slice chunk.
			{
				ase_slice_chunk_t chunk;
				CUTE_ASEPRITE_MEMSET(&chunk, 0, sizeof(chunk));
				chunk.slice_count = s_read_uint32(ase);
				chunk.flags = s_read_uint32(ase);
				s_skip(ase, sizeof(uint32_t)); // Reserved.
				chunk.name = s_read_string(ase);
				chunk.slices = (ase_slice_t*)CUTE_ASEPRITE_ALLOC(sizeof(ase_slice_t) * chunk.slice_count, mem_ctx);
				for (int k = 0; k < (int)chunk.slice_count; ++k) {
					ase_slice_t slice;
					slice.frame_number = s_read_uint32(ase);
					slice.origin_x = s_read_int16(ase);
					slice.origin_y = s_read_int16(ase);
					slice.w = s_read_uint32(ase);
					slice.h = s_read_uint32(ase);
					if (chunk.flags & 1) {
						// It's a 9-patches slice.
						slice.center_x = s_read_int16(ase);
						slice.center_y = s_read_int16(ase);
						slice.center_w = s_read_uint32(ase);
						slice.center_h = s_read_uint32(ase);
					} else if (chunk.flags & 2) {
						// Has pivot information.
						slice.pivot_x = s_read_int16(ase);
						slice.pivot_y = s_read_int16(ase);
					}
				}
			}	break;

			default:
				s_skip(ase, chunk_size);
				break;
			}
		}
	}

	return NULL;
}

void cute_aseprite_free(cute_aseprite_t* aseprite)
{
}

#endif // CUTE_ASEPRITE_IMPLEMENTATION_ONCE
#endif // CUTE_ASEPRITE_IMPLEMENTATION
