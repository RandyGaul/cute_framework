/*
	------------------------------------------------------------------------------
		Licensing information can be found at the end of the file.
	------------------------------------------------------------------------------

	cute_dds.h - v1.00

	To create implementation (the function definitions)
		#define CUTE_DDS_IMPLEMENTATION
	in *one* C/CPP file (translation unit) that includes this file


	SUMMARY:

		A DDS container parser in a single header, the sibling of cute_png.h and
		cute_jpg.h. DDS is the de-facto interchange file for GPU-compressed
		textures: block-compressed pixels (BC1-BC7) with their full mip chains,
		exactly as the GPU consumes them. This header parses the container and
		hands back zero-copy views of each surface -- it never decodes blocks,
		because the entire point is uploading them compressed.

		Supported: BC1/BC2/BC3 (legacy DXT1/DXT3/DXT5 FourCC and DX10 header),
		BC4/BC5 (ATI1/ATI2/BC4U/BC5U and DX10), BC6H and BC7 (DX10 only, as
		they have no legacy FourCC), sRGB variants via DX10 DXGI formats,
		uncompressed 32-bit RGBA/BGRA via legacy pixel masks or DX10, mip
		chains, cube maps (all six faces required), and DX10 texture arrays.

		Rejected with a clear `cd_error_reason`: volume textures, partial cube
		maps, packed/paletted legacy formats, and any file whose surfaces don't
		fit inside the bytes provided. Parsing is fully bounds-checked with
		64-bit size math -- no length field is ever trusted.

	EXAMPLES:

		Parsing a DDS from memory (views point into `memory` -- keep it alive):
			cd_dds_t dds = cd_parse_dds_mem(memory, size);
			if (!dds.slice_count) printf("%s\n", cd_error_reason);
			for (int mip = 0; mip < dds.mip_count; mip++) {
				cd_slice_t* s = cd_slice(&dds, 0, mip);
				upload(s->data, s->size, s->w, s->h, mip);
			}
			cd_free_dds(&dds);

		Loading from disk (the file's bytes ride along and free with the parse):
			cd_dds_t dds = cd_load_dds("textures/rock.dds");
			...
			cd_free_dds(&dds);

	CUSTOMIZATION

		There are various macros in this header you can customize by defining them
		before including cute_dds.h. Simply define one to override the default behavior.

			CUTE_DDS_ALLOC
			CUTE_DDS_FREE
			CUTE_DDS_MEMCPY
			CUTE_DDS_NO_STDIO

	Revision history:
		1.00 (08/04/2026) initial release
*/

#if !defined(CUTE_DDS_H)

#include <stdint.h>

// Read this after a parse returns a zero slice_count.
extern const char* cd_error_reason;

typedef enum cd_format_t
{
	CD_FORMAT_UNKNOWN,
	CD_FORMAT_RGBA8,       // 32-bit R8G8B8A8, unsigned normalized.
	CD_FORMAT_RGBA8_SRGB,
	CD_FORMAT_BGRA8,       // 32-bit B8G8R8A8 (the classic D3D9 layout).
	CD_FORMAT_BGRA8_SRGB,
	CD_FORMAT_BC1,         // 8 bytes per 4x4 block, 1-bit alpha.
	CD_FORMAT_BC1_SRGB,
	CD_FORMAT_BC2,         // 16 bytes per block, explicit 4-bit alpha.
	CD_FORMAT_BC2_SRGB,
	CD_FORMAT_BC3,         // 16 bytes per block, interpolated alpha.
	CD_FORMAT_BC3_SRGB,
	CD_FORMAT_BC4,         // 8 bytes per block, single channel.
	CD_FORMAT_BC5,         // 16 bytes per block, two channels.
	CD_FORMAT_BC6H_UF16,   // 16 bytes per block, unsigned HDR.
	CD_FORMAT_BC6H_SF16,   // 16 bytes per block, signed HDR.
	CD_FORMAT_BC7,         // 16 bytes per block, high-quality RGBA.
	CD_FORMAT_BC7_SRGB,
} cd_format_t;

// One surface: a single mip of a single face/array element. `data` points into
// the memory handed to the parser (or the file bytes owned by the parse).
typedef struct cd_slice_t
{
	const void* data;
	int size;
	int w, h;
} cd_slice_t;

typedef struct cd_dds_t
{
	cd_format_t format;
	int w, h;              // Top mip dimensions.
	int mip_count;         // At least 1.
	int face_count;        // 1, or 6 * array size for cube maps, or the array size.
	int slice_count;       // face_count * mip_count; zero when parsing failed.
	int is_cubemap;        // Nonzero when face_count covers +X -X +Y -Y +Z -Z (times array size).
	cd_slice_t* slices;    // One allocation, face-major: face * mip_count + mip.
	void* file_data;       // Owned file bytes (cd_load_dds only); freed by cd_free_dds.
} cd_dds_t;

// Parses DDS bytes into zero-copy surface views. On failure returns a zeroed
// struct (slice_count == 0) and sets `cd_error_reason`.
cd_dds_t cd_parse_dds_mem(const void* data, int size);

#if !defined(CUTE_DDS_NO_STDIO)
// Reads the whole file and parses it; the bytes free together with the parse.
cd_dds_t cd_load_dds(const char* path);
#endif

// The slice for (face, mip), or NULL if out of range.
cd_slice_t* cd_slice(cd_dds_t* dds, int face, int mip);

void cd_free_dds(cd_dds_t* dds);

// Bytes per 4x4 block for compressed formats, or 0 for uncompressed ones.
int cd_block_size(cd_format_t format);

#define CUTE_DDS_H
#endif

#ifdef CUTE_DDS_IMPLEMENTATION
#ifndef CUTE_DDS_IMPLEMENTATION_ONCE
#define CUTE_DDS_IMPLEMENTATION_ONCE

#if !defined(CUTE_DDS_ALLOC)
	#include <stdlib.h>
	#define CUTE_DDS_ALLOC(size) malloc(size)
	#define CUTE_DDS_FREE(ptr) free(ptr)
#endif

#if !defined(CUTE_DDS_MEMCPY)
	#include <string.h>
	#define CUTE_DDS_MEMCPY memcpy
#endif

const char* cd_error_reason;

#define CD_FAIL(reason) do { cd_error_reason = reason; goto cd_err; } while (0)

// Header flag and caps bits (the subset that matters; everything else is advisory).
#define CD_DDPF_FOURCC      0x4
#define CD_DDPF_RGB         0x40
#define CD_DDSCAPS2_CUBEMAP 0x200
#define CD_DDSCAPS2_CUBEMAP_ALL_FACES 0xfc00
#define CD_DDSCAPS2_VOLUME  0x200000

#define CD_FOURCC(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

// DXGI format codes consumed from the DX10 extension header.
#define CD_DXGI_R8G8B8A8_UNORM      28
#define CD_DXGI_R8G8B8A8_UNORM_SRGB 29
#define CD_DXGI_B8G8R8A8_UNORM      87
#define CD_DXGI_B8G8R8A8_UNORM_SRGB 91
#define CD_DXGI_BC1_UNORM           71
#define CD_DXGI_BC1_UNORM_SRGB      72
#define CD_DXGI_BC2_UNORM           74
#define CD_DXGI_BC2_UNORM_SRGB      75
#define CD_DXGI_BC3_UNORM           77
#define CD_DXGI_BC3_UNORM_SRGB      78
#define CD_DXGI_BC4_UNORM           80
#define CD_DXGI_BC5_UNORM           83
#define CD_DXGI_BC6H_UF16           95
#define CD_DXGI_BC6H_SF16           96
#define CD_DXGI_BC7_UNORM           98
#define CD_DXGI_BC7_UNORM_SRGB      99

#define CD_DX10_DIMENSION_TEXTURE2D 3
#define CD_DX10_MISC_TEXTURECUBE    0x4

static uint32_t cd_u32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

int cd_block_size(cd_format_t format)
{
	switch (format) {
	case CD_FORMAT_BC1: case CD_FORMAT_BC1_SRGB:
	case CD_FORMAT_BC4: return 8;
	case CD_FORMAT_BC2: case CD_FORMAT_BC2_SRGB:
	case CD_FORMAT_BC3: case CD_FORMAT_BC3_SRGB:
	case CD_FORMAT_BC5:
	case CD_FORMAT_BC6H_UF16: case CD_FORMAT_BC6H_SF16:
	case CD_FORMAT_BC7: case CD_FORMAT_BC7_SRGB: return 16;
	default: return 0;
	}
}

// Byte size of one w-by-h surface in `format`; 64-bit so hostile dimensions can't wrap.
static uint64_t cd_surface_size(cd_format_t format, uint64_t w, uint64_t h)
{
	int block = cd_block_size(format);
	if (block) return ((w + 3) / 4) * ((h + 3) / 4) * (uint64_t)block;
	return w * h * 4;
}

cd_dds_t cd_parse_dds_mem(const void* data, int size)
{
	// All locals up front: the CD_FAIL goto must not jump over an initialization
	// (this header compiles as C++ inside engines).
	cd_dds_t out;
	cd_dds_t zero = { CD_FORMAT_UNKNOWN };
	const uint8_t* p = (const uint8_t*)data;
	const uint8_t* h = 0;
	cd_slice_t* slices = 0;
	uint32_t height = 0, width = 0, mip_count = 0;
	uint32_t pf_flags = 0, fourcc = 0, rgb_bits = 0;
	uint32_t rmask = 0, gmask = 0, bmask = 0, amask = 0, caps2 = 0;
	uint32_t array_size = 1;
	int offset = 4 + 124;
	int is_cube = 0;
	cd_format_t format = CD_FORMAT_UNKNOWN;
	uint64_t face_count = 0, slice_count = 0;
	out = zero;

	// Magic + the fixed 124-byte DDS_HEADER.
	if (!p || size < 4 + 124) CD_FAIL("file too small for a DDS header");
	if (cd_u32(p) != CD_FOURCC('D', 'D', 'S', ' ')) CD_FAIL("not a DDS file (bad magic)");
	// DDS_HEADER layout: dwSize(0) dwFlags(4) dwHeight(8) dwWidth(12) dwPitchOrLinearSize(16)
	// dwDepth(20) dwMipMapCount(24) dwReserved1[11](28) ddspf(72..103) dwCaps(104) dwCaps2(108).
	h = p + 4;
	if (cd_u32(h + 0) != 124) CD_FAIL("bad DDS_HEADER size field");
	height = cd_u32(h + 8);
	width = cd_u32(h + 12);
	mip_count = cd_u32(h + 24);
	pf_flags = cd_u32(h + 76);
	fourcc = cd_u32(h + 80);
	rgb_bits = cd_u32(h + 84);
	rmask = cd_u32(h + 88);
	gmask = cd_u32(h + 92);
	bmask = cd_u32(h + 96);
	amask = cd_u32(h + 100);
	caps2 = cd_u32(h + 108);

	if (width == 0 || height == 0) CD_FAIL("zero width or height");
	if (width > 65536 || height > 65536) CD_FAIL("unreasonable dimensions");
	if (mip_count == 0) mip_count = 1;
	if (mip_count > 17) CD_FAIL("mip count exceeds any possible chain"); // log2(65536) + 1.
	if (caps2 & CD_DDSCAPS2_VOLUME) CD_FAIL("volume textures are not supported");

	// Pixel format: DX10 extension header, legacy FourCC, or raw RGB masks.
	if ((pf_flags & CD_DDPF_FOURCC) && fourcc == CD_FOURCC('D', 'X', '1', '0')) {
		if (size < offset + 20) CD_FAIL("file too small for the DX10 header");
		const uint8_t* x = p + offset;
		uint32_t dxgi = cd_u32(x + 0);
		uint32_t dimension = cd_u32(x + 4);
		uint32_t misc = cd_u32(x + 8);
		array_size = cd_u32(x + 12);
		offset += 20;
		if (dimension != CD_DX10_DIMENSION_TEXTURE2D) CD_FAIL("only 2D (and cube) DX10 dimensions are supported");
		if (array_size == 0) CD_FAIL("zero DX10 array size");
		if (array_size > 2048) CD_FAIL("unreasonable DX10 array size");
		if (misc & CD_DX10_MISC_TEXTURECUBE) is_cube = 1;
		switch (dxgi) {
		case CD_DXGI_R8G8B8A8_UNORM:      format = CD_FORMAT_RGBA8; break;
		case CD_DXGI_R8G8B8A8_UNORM_SRGB: format = CD_FORMAT_RGBA8_SRGB; break;
		case CD_DXGI_B8G8R8A8_UNORM:      format = CD_FORMAT_BGRA8; break;
		case CD_DXGI_B8G8R8A8_UNORM_SRGB: format = CD_FORMAT_BGRA8_SRGB; break;
		case CD_DXGI_BC1_UNORM:           format = CD_FORMAT_BC1; break;
		case CD_DXGI_BC1_UNORM_SRGB:      format = CD_FORMAT_BC1_SRGB; break;
		case CD_DXGI_BC2_UNORM:           format = CD_FORMAT_BC2; break;
		case CD_DXGI_BC2_UNORM_SRGB:      format = CD_FORMAT_BC2_SRGB; break;
		case CD_DXGI_BC3_UNORM:           format = CD_FORMAT_BC3; break;
		case CD_DXGI_BC3_UNORM_SRGB:      format = CD_FORMAT_BC3_SRGB; break;
		case CD_DXGI_BC4_UNORM:           format = CD_FORMAT_BC4; break;
		case CD_DXGI_BC5_UNORM:           format = CD_FORMAT_BC5; break;
		case CD_DXGI_BC6H_UF16:           format = CD_FORMAT_BC6H_UF16; break;
		case CD_DXGI_BC6H_SF16:           format = CD_FORMAT_BC6H_SF16; break;
		case CD_DXGI_BC7_UNORM:           format = CD_FORMAT_BC7; break;
		case CD_DXGI_BC7_UNORM_SRGB:      format = CD_FORMAT_BC7_SRGB; break;
		default: CD_FAIL("unsupported DXGI format");
		}
	} else if (pf_flags & CD_DDPF_FOURCC) {
		if (fourcc == CD_FOURCC('D', 'X', 'T', '1')) format = CD_FORMAT_BC1;
		else if (fourcc == CD_FOURCC('D', 'X', 'T', '2') || fourcc == CD_FOURCC('D', 'X', 'T', '3')) format = CD_FORMAT_BC2;
		else if (fourcc == CD_FOURCC('D', 'X', 'T', '4') || fourcc == CD_FOURCC('D', 'X', 'T', '5')) format = CD_FORMAT_BC3;
		else if (fourcc == CD_FOURCC('A', 'T', 'I', '1') || fourcc == CD_FOURCC('B', 'C', '4', 'U')) format = CD_FORMAT_BC4;
		else if (fourcc == CD_FOURCC('A', 'T', 'I', '2') || fourcc == CD_FOURCC('B', 'C', '5', 'U')) format = CD_FORMAT_BC5;
		else CD_FAIL("unsupported FourCC (BC6H/BC7 files must use the DX10 header)");
	} else if (pf_flags & CD_DDPF_RGB) {
		if (rgb_bits != 32) CD_FAIL("only 32-bit uncompressed DDS is supported");
		if (rmask == 0x000000ff && gmask == 0x0000ff00 && bmask == 0x00ff0000) format = CD_FORMAT_RGBA8;
		else if (rmask == 0x00ff0000 && gmask == 0x0000ff00 && bmask == 0x000000ff) format = CD_FORMAT_BGRA8;
		else CD_FAIL("unsupported uncompressed channel masks");
		(void)amask; // With or without alpha bits, the memory layout reads the same.
	} else {
		CD_FAIL("unsupported pixel format (no FourCC and no RGB masks)");
	}

	// Legacy cube maps flag through caps2. Partial cubes were legal in D3D9; nothing
	// modern consumes them, so require all six faces.
	if (caps2 & CD_DDSCAPS2_CUBEMAP) {
		if ((caps2 & CD_DDSCAPS2_CUBEMAP_ALL_FACES) != CD_DDSCAPS2_CUBEMAP_ALL_FACES) CD_FAIL("partial cube maps are not supported");
		is_cube = 1;
	}

	face_count = (uint64_t)array_size * (is_cube ? 6 : 1);
	slice_count = face_count * mip_count;
	if (slice_count > 65536) CD_FAIL("unreasonable slice count");

	// Walk every surface, validating that each fits in the remaining bytes.
	slices = (cd_slice_t*)CUTE_DDS_ALLOC(sizeof(cd_slice_t) * (size_t)slice_count);
	if (!slices) CD_FAIL("out of memory");
	{
		uint64_t cursor = (uint64_t)offset;
		for (uint64_t f = 0; f < face_count; f++) {
			uint32_t w = width, hgt = height;
			for (uint32_t m = 0; m < mip_count; m++) {
				uint64_t sz = cd_surface_size(format, w, hgt);
				if (cursor + sz > (uint64_t)size) CD_FAIL("file too small for its surfaces");
				cd_slice_t* s = slices + f * mip_count + m;
				s->data = p + cursor;
				s->size = (int)sz;
				s->w = (int)w;
				s->h = (int)hgt;
				cursor += sz;
				w = w > 1 ? w >> 1 : 1;
				hgt = hgt > 1 ? hgt >> 1 : 1;
			}
		}
	}

	out.format = format;
	out.w = (int)width;
	out.h = (int)height;
	out.mip_count = (int)mip_count;
	out.face_count = (int)face_count;
	out.slice_count = (int)slice_count;
	out.is_cubemap = is_cube;
	out.slices = slices;
	return out;

cd_err:
	if (slices) CUTE_DDS_FREE(slices);
	return zero;
}

cd_slice_t* cd_slice(cd_dds_t* dds, int face, int mip)
{
	if (!dds || !dds->slices) return 0;
	if (face < 0 || face >= dds->face_count) return 0;
	if (mip < 0 || mip >= dds->mip_count) return 0;
	return dds->slices + face * dds->mip_count + mip;
}

void cd_free_dds(cd_dds_t* dds)
{
	if (!dds) return;
	if (dds->slices) CUTE_DDS_FREE(dds->slices);
	if (dds->file_data) CUTE_DDS_FREE(dds->file_data);
	cd_dds_t zero = { CD_FORMAT_UNKNOWN };
	*dds = zero;
}

#if !defined(CUTE_DDS_NO_STDIO)
#include <stdio.h>

cd_dds_t cd_load_dds(const char* path)
{
	cd_dds_t zero = { CD_FORMAT_UNKNOWN };
	FILE* fp = fopen(path, "rb");
	if (!fp) { cd_error_reason = "unable to open file"; return zero; }
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (sz <= 0 || sz > 0x7fffffff) { fclose(fp); cd_error_reason = "bad file size"; return zero; }
	void* data = CUTE_DDS_ALLOC((size_t)sz);
	if (!data) { fclose(fp); cd_error_reason = "out of memory"; return zero; }
	size_t read = fread(data, 1, (size_t)sz, fp);
	fclose(fp);
	if (read != (size_t)sz) { CUTE_DDS_FREE(data); cd_error_reason = "unable to read file"; return zero; }
	cd_dds_t dds = cd_parse_dds_mem(data, (int)sz);
	if (!dds.slice_count) { CUTE_DDS_FREE(data); return dds; }
	dds.file_data = data;
	return dds;
}
#endif

#endif // CUTE_DDS_IMPLEMENTATION_ONCE
#endif // CUTE_DDS_IMPLEMENTATION

/*
	------------------------------------------------------------------------------
	This software is available under 2 licenses - you may choose the one you like.
	------------------------------------------------------------------------------
	ALTERNATIVE A - zlib license
	Copyright (c) 2026 Randy Gaul https://randygaul.github.io/
	This software is provided 'as-is', without any express or implied warranty.
	In no event will the authors be held liable for any damages arising from
	the use of this software.
	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	  1. The origin of this software must not be misrepresented; you must not
	     claim that you wrote the original software. If you use this software
	     in a product, an acknowledgment in the product documentation would be
	     appreciated but is not required.
	  2. Altered source versions must be plainly marked as such, and must not
	     be misrepresented as being the original software.
	  3. This notice may not be removed or altered from any source distribution.
	------------------------------------------------------------------------------
	ALTERNATIVE B - Public Domain (www.unlicense.org)
	This is free and unencumbered software released into the public domain.
	Anyone is free to copy, modify, publish, use, compile, sell, or distribute
	this software, either in source code form or as a compiled binary, for any
	purpose, commercial or non-commercial, and by any means.
	In jurisdictions that recognize copyright laws, the author or authors of
	this software dedicate any and all copyright interest in the software to
	the public domain. We make this dedication for the benefit of the public
	at large and to the detriment of our heirs and successors. We intend this
	dedication to be an overt act of relinquishment in perpetuity of all
	present and future rights to this software under copyright law.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	------------------------------------------------------------------------------
*/
