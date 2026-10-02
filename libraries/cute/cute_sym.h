/*
	------------------------------------------------------------------------------
		Licensing information can be found at the end of the file.
	------------------------------------------------------------------------------

	cute_sym.h - v0.01

	To create implementation (the function definitions)
		#define CUTE_SYM_IMPLEMENTATION
	in *one* C/CPP file (translation unit) that includes this file

	To also get a command line tool, define CUTE_SYM_MAIN in that same file: it then
	defines main() as `return sym_main(argc, argv)`.


	SUMMARY:

		The symbol side of crash reporting, the sibling of cute_crash.h. A game ships
		with no debug information; its crash reports carry raw addresses as module +
		offset plus each module's build id. This header turns a build's debug
		information (PDB on Windows, DWARF in a dSYM, the debug map, or an ELF) into
		one small table per module -- functions, line rows, inline sites, keyed by the
		build id -- and resolves a report against such tables, wherever they are: a
		.sym file beside the module, a symbol store on the developer's machine, or a
		slot inside the executable itself (sym_embed). The table format is the contract
		between the tool and the runtime; cute_crash.h carries its own reader of it.

		The table design follows crash-where by bullno1 (https://github.com/bullno1/
		crash-where), the original author: a build-id keyed, sorted, immutable file of
		functions, line rows and inline sites with normalized names, made once at build
		time where the toolchain is, and read by a lookup that never knows which debug
		format it came from. This header is a self-contained reimplementation: it reads
		PDB, DWARF, PE, ELF and Mach-O itself, with no toolchain or library behind it,
		so it builds and runs anywhere a C compiler does.

		No dependencies beyond libc. Valid C99 and C++.


	USAGE:

		Build a table from a binary and its debug info, beside the binary:

			cute_sym game.exe                     writes game.sym (reads game.pdb)
			cute_sym game.exe --embed             patches the table into game.exe's slot
			cute_sym game --debug game.dSYM/Contents/Resources/DWARF/game -o store/game.sym

		Resolve and read a report:

			cute_sym resolve crash.json --symbols store/
			cute_sym print crash.json

		As a library (the loader of a hot-reloaded DLL, say):

			sym_table t;
			if (sym_build("scripts.dll", NULL, &t, NULL, NULL)) {
				sym_write(&t, "scripts.sym");
				sym_free(&t);
			}


	THE TABLE (CUTESYM version 1, little-endian):

		header, 96 bytes:
		  0   magic        u8[8]  "CUTESYM\0"
		  8   version      u32    1
		  12  arch         u32    1 x86_64, 2 aarch64, 3 x86, 4 wasm32
		  16  build_id     u8[20]
		  36  build_id_len u8
		  37  flags        u8     bit0 has lines, bit1 has inlines
		  38  reserved     u8[2]
		  40  func_count   u32
		  44  line_count   u32
		  48  inline_count u32
		  52  file_count   u32
		  56  strings_len  u32
		  60  off_funcs    u32    byte offsets from the file start; every section 8-aligned
		  64  off_lines    u32
		  68  off_inlines  u32
		  72  off_files    u32
		  76  off_strings  u32
		  80  total_len    u32
		  84  reserved     u8[12]
		funcs    sym_func   (16 bytes)  sorted by start, non-overlapping
		lines    sym_line   (24 bytes)  sorted by start, non-overlapping
		inlines  sym_inline (32 bytes)  sorted by start; nested ranges; parent = index of the
		                                enclosing inline range or SYM_NO_PARENT
		files    u32 string offsets [file_count]
		strings  NUL-terminated; offset 0 is "" (so 0 means "no name")

		Addresses: PE tables hold RVAs (pc - module base); Mach-O tables hold link-time
		vmaddrs (pc - dyld slide); ELF tables hold link-time vaddrs (pc - load bias). A
		report's `offset` uses the same convention, so lookup needs no further arithmetic.

		Build ids: PE = the 16 PDB GUID bytes as the file stores them followed by the age as
		4 big-endian bytes (20 bytes); Mach-O = LC_UUID (16); ELF = the GNU build-id note.

		The embed slot: a binary built with cute_crash.h's CUTE_CRASH_SYM_RESERVE carries a
		read-only array starting "CUTESYMSLOT\0" + u32 capacity + zeros. sym_embed finds
		it in the file and overwrites the zeros with the table, before code signing.
*/

#ifndef CUTE_SYM_H
#define CUTE_SYM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYM_VERSION 1
#define SYM_NO_PARENT 0xFFFFFFFFu

enum { SYM_ARCH_NONE = 0, SYM_ARCH_X86_64 = 1, SYM_ARCH_AARCH64 = 2, SYM_ARCH_X86 = 3, SYM_ARCH_WASM32 = 4 };

// The three record kinds as they lie in the file. Trailing padding is stored as zeros, so
// sizeof() is the on-disk size on every compiler (16, 24, 32).
typedef struct sym_func { uint64_t start; uint32_t size; uint32_t name; } sym_func;
typedef struct sym_line { uint64_t start; uint32_t len; uint32_t file; uint32_t line; uint32_t pad; } sym_line;
typedef struct sym_inline { uint64_t start; uint32_t len; uint32_t callee; uint32_t call_file; uint32_t call_line; uint32_t parent; uint32_t pad; } sym_inline;

// A parsed view over the table bytes. Zero-copy: the record pointers point into `bytes`,
// which the caller owns unless `owned` (sym_read_file, sym_build), in which case sym_free
// releases it. The lookups read records through byte copies, so `bytes` may be unaligned.
typedef struct sym_table {
	const unsigned char* bytes;
	size_t len;
	int arch;
	unsigned char build_id[20];
	int build_id_len;
	const sym_func* funcs; uint32_t func_count;
	const sym_line* lines; uint32_t line_count;
	const sym_inline* inlines; uint32_t inline_count;
	const uint32_t* files; uint32_t file_count;
	const char* strings; uint32_t strings_len;
	bool owned;
} sym_table;

// One resolved location. From sym_lookup's `out`: the function containing the address and the
// line row at it (the innermost code's own file:line). From its `inlined` list: for each inline
// range containing the address, innermost first, the callee and the call site in its caller.
typedef struct sym_frame { const char* function; const char* file; uint32_t line; } sym_frame;

typedef void (*sym_log_fn)(void* udata, const char* msg);

// Builds a table from a binary and its debug information. debug_path NULL finds it: PE -> the
// PDB the debug directory names, then beside the binary by basename; Mach-O -> the dSYM beside
// the binary, else the debug map and its object files; ELF -> the file itself, then
// .gnu_debuglink beside it. The table is owned (sym_free).
bool sym_build(const char* binary_path, const char* debug_path, sym_table* out, sym_log_fn log, void* udata);
bool sym_write(const sym_table* t, const char* path);
bool sym_embed(const sym_table* t, const char* binary_path, sym_log_fn log, void* udata); // Patches the slot. Before signing.
bool sym_read(const void* bytes, size_t len, sym_table* out);  // Validates the whole layout.
bool sym_read_file(const char* path, sym_table* out);
void sym_free(sym_table* t);
bool sym_lookup(const sym_table* t, uint64_t addr, sym_frame* out, sym_frame* inlined, int inlined_cap, int* inlined_count);
bool sym_resolve(const char* report_path, const char* sym_dir, sym_log_fn log, void* udata); // In place, idempotent.
bool sym_print(const char* report_path, FILE* out);
int sym_main(int argc, char** argv);

#ifdef __cplusplus
}
#endif

#endif // CUTE_SYM_H

//--------------------------------------------------------------------------------------------------

#ifdef CUTE_SYM_IMPLEMENTATION
#ifndef CUTE_SYM_IMPLEMENTATION_ONCE
#define CUTE_SYM_IMPLEMENTATION_ONCE

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable: 4996) // fopen, strncpy: the portable forms, used with explicit bounds.
#endif
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#define SYM_HEADER_BYTES 96
#define SYM_SLOT_MAGIC "CUTESYMSLOT"
#define SYM_SLOT_HEAD 16

//--------------------------------------------------------------------------------------------------
// Bytes, files, logging.

static uint16_t s_rd16(const unsigned char* p) { uint16_t v; memcpy(&v, p, 2); return v; }
static uint32_t s_rd32(const unsigned char* p) { uint32_t v; memcpy(&v, p, 4); return v; }
static uint64_t s_rd64(const unsigned char* p) { uint64_t v; memcpy(&v, p, 8); return v; }
static void s_wr32(unsigned char* p, uint32_t v) { memcpy(p, &v, 4); }
static void s_wr64(unsigned char* p, uint64_t v) { memcpy(p, &v, 8); }

void sym_logf(sym_log_fn log, void* udata, const char* fmt, ...)
{
	char buf[1024];
	va_list args;
	if (!log) return;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	buf[sizeof(buf) - 1] = 0;
	log(udata, buf);
}

static unsigned char* s_read_file(const char* path, size_t* len)
{
	FILE* f = fopen(path, "rb");
	unsigned char* bytes;
	long n;
	if (!f) return NULL;
	if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
	n = ftell(f);
	if (n < 0) { fclose(f); return NULL; }
	fseek(f, 0, SEEK_SET);
	bytes = (unsigned char*)malloc((size_t)n + 16);
	if (!bytes) { fclose(f); return NULL; }
	if (n > 0 && fread(bytes, 1, (size_t)n, f) != (size_t)n) { free(bytes); fclose(f); return NULL; }
	fclose(f);
	bytes[n] = 0;
	*len = (size_t)n;
	return bytes;
}

static bool s_write_file(const char* path, const void* bytes, size_t len)
{
	FILE* f = fopen(path, "wb");
	bool ok;
	if (!f) return false;
	ok = len == 0 || fwrite(bytes, 1, len, f) == len;
	fclose(f);
	return ok;
}

static const char* s_basename(const char* path)
{
	const char* p = path + strlen(path);
	while (p > path && p[-1] != '/' && p[-1] != '\\') --p;
	return p;
}

static void s_dirname(const char* path, char* out, size_t cap)
{
	const char* b = s_basename(path);
	size_t n = (size_t)(b - path);
	if (n >= cap) n = cap - 1;
	memcpy(out, path, n);
	out[n] = 0;
	if (n == 0) { out[0] = '.'; out[1] = 0; }
}

static void s_hex(const unsigned char* bytes, int n, char* out)
{
	static const char* digits = "0123456789abcdef";
	int i;
	for (i = 0; i < n; ++i) { out[i * 2] = digits[bytes[i] >> 4]; out[i * 2 + 1] = digits[bytes[i] & 15]; }
	out[n * 2] = 0;
}

static int s_unhex(const char* s, unsigned char* out, int cap)
{
	int n = 0;
	while (s[0] && s[1] && n < cap) {
		int hi = isxdigit((unsigned char)s[0]) ? (isdigit((unsigned char)s[0]) ? s[0] - '0' : (tolower((unsigned char)s[0]) - 'a' + 10)) : -1;
		int lo = isxdigit((unsigned char)s[1]) ? (isdigit((unsigned char)s[1]) ? s[1] - '0' : (tolower((unsigned char)s[1]) - 'a' + 10)) : -1;
		if (hi < 0 || lo < 0) return -1;
		out[n++] = (unsigned char)((hi << 4) | lo);
		s += 2;
	}
	return s[0] ? -1 : n;
}

//--------------------------------------------------------------------------------------------------
// SHA-1, for the symbolic signature.

typedef struct s_sha1 { uint32_t h[5]; unsigned char block[64]; size_t block_len; uint64_t total; } s_sha1;

static uint32_t s_rol(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

static void s_sha1_block(s_sha1* s, const unsigned char* p)
{
	uint32_t w[80], a, b, c, d, e;
	int i;
	for (i = 0; i < 16; ++i) w[i] = ((uint32_t)p[i * 4] << 24) | ((uint32_t)p[i * 4 + 1] << 16) | ((uint32_t)p[i * 4 + 2] << 8) | p[i * 4 + 3];
	for (; i < 80; ++i) w[i] = s_rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
	a = s->h[0]; b = s->h[1]; c = s->h[2]; d = s->h[3]; e = s->h[4];
	for (i = 0; i < 80; ++i) {
		uint32_t f, k, t;
		if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
		else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
		else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
		else { f = b ^ c ^ d; k = 0xCA62C1D6; }
		t = s_rol(a, 5) + f + e + k + w[i];
		e = d; d = c; c = s_rol(b, 30); b = a; a = t;
	}
	s->h[0] += a; s->h[1] += b; s->h[2] += c; s->h[3] += d; s->h[4] += e;
}

static void s_sha1_init(s_sha1* s)
{
	s->h[0] = 0x67452301; s->h[1] = 0xEFCDAB89; s->h[2] = 0x98BADCFE; s->h[3] = 0x10325476; s->h[4] = 0xC3D2E1F0;
	s->block_len = 0; s->total = 0;
}

static void s_sha1_update(s_sha1* s, const void* data, size_t len)
{
	const unsigned char* p = (const unsigned char*)data;
	s->total += len;
	while (len > 0) {
		size_t n = 64 - s->block_len;
		if (n > len) n = len;
		memcpy(s->block + s->block_len, p, n);
		s->block_len += n; p += n; len -= n;
		if (s->block_len == 64) { s_sha1_block(s, s->block); s->block_len = 0; }
	}
}

static void s_sha1_final(s_sha1* s, unsigned char out[20])
{
	uint64_t bits = s->total * 8;
	unsigned char pad = 0x80, zero = 0, lenb[8];
	int i;
	s_sha1_update(s, &pad, 1);
	while (s->block_len != 56) s_sha1_update(s, &zero, 1);
	for (i = 0; i < 8; ++i) lenb[i] = (unsigned char)(bits >> (56 - i * 8));
	s_sha1_update(s, lenb, 8);
	for (i = 0; i < 5; ++i) { out[i * 4] = (unsigned char)(s->h[i] >> 24); out[i * 4 + 1] = (unsigned char)(s->h[i] >> 16); out[i * 4 + 2] = (unsigned char)(s->h[i] >> 8); out[i * 4 + 3] = (unsigned char)s->h[i]; }
}

//--------------------------------------------------------------------------------------------------
// The sink: readers push functions, line rows and inline sites; the builder interns, sorts,
// resolves overlaps (the earlier push wins, later ones are clipped around it), links inline
// parents by containment, and serializes.

typedef struct sym_sink sym_sink;
void sym_sink_module(sym_sink* s, int arch, const unsigned char* build_id, int build_id_len);
void sym_sink_function(sym_sink* s, uint64_t start, uint32_t size, const char* name);
void sym_sink_line(sym_sink* s, uint64_t start, uint32_t len, const char* file, uint32_t line);
void sym_sink_line_end(sym_sink* s, uint64_t end);
void sym_sink_inline(sym_sink* s, uint64_t start, uint32_t len, const char* callee, const char* call_file, uint32_t call_line, int depth);

typedef struct s_func_rec { uint64_t start, end; uint32_t name, order; } s_func_rec;
typedef struct s_line_rec { uint64_t start, end; uint32_t file, line, order; } s_line_rec;
typedef struct s_inl_rec { uint64_t start, end; uint32_t callee, call_file, call_line, parent; int depth; uint32_t order; } s_inl_rec;

struct sym_sink {
	int arch;
	unsigned char build_id[20];
	int build_id_len;
	char* strs; size_t strs_len, strs_cap;
	uint32_t* slots; size_t slot_cap, slot_count; // Open addressing over string offsets.
	s_func_rec* funcs; size_t func_count, func_cap;
	s_line_rec* lines; size_t line_count, line_cap;
	s_inl_rec* inls; size_t inl_count, inl_cap;
	long open_line; // The last pushed row when it has no length yet, else -1.
	uint32_t order;
	bool failed;
};

static void* s_grow(void* p, size_t* cap, size_t count, size_t elem)
{
	size_t ncap;
	void* np;
	if (count < *cap) return p;
	ncap = *cap ? *cap * 2 : 256;
	while (ncap <= count) ncap *= 2;
	np = realloc(p, ncap * elem);
	if (!np) return NULL;
	*cap = ncap;
	return np;
}

static uint32_t s_str_hash(const char* s, size_t n)
{
	uint32_t h = 2166136261u;
	size_t i;
	for (i = 0; i < n; ++i) { h ^= (unsigned char)s[i]; h *= 16777619u; }
	return h ? h : 1;
}

static sym_sink* s_sink_make(void)
{
	sym_sink* s = (sym_sink*)calloc(1, sizeof(sym_sink));
	if (!s) return NULL;
	s->strs_cap = 4096;
	s->strs = (char*)malloc(s->strs_cap);
	if (!s->strs) { free(s); return NULL; }
	s->strs[0] = 0;
	s->strs_len = 1; // Offset 0 is "".
	s->open_line = -1;
	return s;
}

static void s_sink_free(sym_sink* s)
{
	if (!s) return;
	free(s->strs); free(s->slots); free(s->funcs); free(s->lines); free(s->inls);
	free(s);
}

static uint32_t s_intern(sym_sink* s, const char* str)
{
	size_t n, i;
	uint32_t h;
	if (!str || !str[0]) return 0;
	n = strlen(str);
	h = s_str_hash(str, n);
	if (s->slot_count * 2 >= s->slot_cap) {
		size_t ncap = s->slot_cap ? s->slot_cap * 2 : 1024, k;
		uint32_t* nslots = (uint32_t*)calloc(ncap, sizeof(uint32_t));
		if (!nslots) { s->failed = true; return 0; }
		for (k = 0; k < s->slot_cap; ++k) {
			if (s->slots[k]) {
				const char* e = s->strs + s->slots[k];
				size_t j = s_str_hash(e, strlen(e)) & (ncap - 1);
				while (nslots[j]) j = (j + 1) & (ncap - 1);
				nslots[j] = s->slots[k];
			}
		}
		free(s->slots);
		s->slots = nslots;
		s->slot_cap = ncap;
	}
	i = h & (s->slot_cap - 1);
	while (s->slots[i]) {
		if (strcmp(s->strs + s->slots[i], str) == 0) return s->slots[i];
		i = (i + 1) & (s->slot_cap - 1);
	}
	if (s->strs_len + n + 1 > s->strs_cap || s->strs_len + n + 1 > 0xFFFFFFFFu) {
		size_t ncap = s->strs_cap * 2;
		char* ns;
		while (ncap < s->strs_len + n + 1) ncap *= 2;
		ns = (char*)realloc(s->strs, ncap);
		if (!ns) { s->failed = true; return 0; }
		s->strs = ns;
		s->strs_cap = ncap;
	}
	memcpy(s->strs + s->strs_len, str, n + 1);
	s->slots[i] = (uint32_t)s->strs_len;
	s->slot_count++;
	s->strs_len += n + 1;
	return s->slots[i];
}

void sym_sink_module(sym_sink* s, int arch, const unsigned char* build_id, int build_id_len)
{
	s->arch = arch;
	if (build_id_len > 20) build_id_len = 20;
	if (build_id_len < 0) build_id_len = 0;
	memset(s->build_id, 0, sizeof(s->build_id));
	if (build_id_len) memcpy(s->build_id, build_id, (size_t)build_id_len);
	s->build_id_len = build_id_len;
}

void sym_sink_function(sym_sink* s, uint64_t start, uint32_t size, const char* name)
{
	s_func_rec* r;
	if (s->failed || size == 0) return;
	s->funcs = (s_func_rec*)s_grow(s->funcs, &s->func_cap, s->func_count, sizeof(s_func_rec));
	if (!s->funcs) { s->failed = true; return; }
	r = &s->funcs[s->func_count++];
	r->start = start; r->end = start + size; r->name = s_intern(s, name); r->order = s->order++;
}

void sym_sink_line_end(sym_sink* s, uint64_t end)
{
	if (s->open_line >= 0) {
		s_line_rec* r = &s->lines[s->open_line];
		r->end = end > r->start ? end : r->start; // Zero length drops at finish.
		s->open_line = -1;
	}
}

void sym_sink_line(sym_sink* s, uint64_t start, uint32_t len, const char* file, uint32_t line)
{
	s_line_rec* r;
	if (s->failed) return;
	sym_sink_line_end(s, start);
	s->lines = (s_line_rec*)s_grow(s->lines, &s->line_cap, s->line_count, sizeof(s_line_rec));
	if (!s->lines) { s->failed = true; return; }
	r = &s->lines[s->line_count++];
	r->start = start; r->end = len ? start + len : start; r->file = s_intern(s, file); r->line = line; r->order = s->order++;
	s->open_line = len ? -1 : (long)(s->line_count - 1);
}

void sym_sink_inline(sym_sink* s, uint64_t start, uint32_t len, const char* callee, const char* call_file, uint32_t call_line, int depth)
{
	s_inl_rec* r;
	if (s->failed || len == 0) return;
	s->inls = (s_inl_rec*)s_grow(s->inls, &s->inl_cap, s->inl_count, sizeof(s_inl_rec));
	if (!s->inls) { s->failed = true; return; }
	r = &s->inls[s->inl_count++];
	r->start = start; r->end = start + len; r->callee = s_intern(s, callee); r->call_file = s_intern(s, call_file);
	r->call_line = call_line; r->depth = depth < 0 ? 0 : depth; r->parent = SYM_NO_PARENT; r->order = s->order++;
}

// Overlap resolution: a sweep over range ends, the active range with the lowest push order
// owning each piece. Pieces of one range re-merge; pieces of different ranges with equal
// payload (`same`) merge too, so a function split by nothing stays one record.
typedef struct s_event { uint64_t pos; uint32_t index; uint8_t is_end; } s_event;

static int s_event_cmp(const void* a, const void* b)
{
	const s_event* x = (const s_event*)a; const s_event* y = (const s_event*)b;
	if (x->pos != y->pos) return x->pos < y->pos ? -1 : 1;
	if (x->is_end != y->is_end) return x->is_end ? -1 : 1; // Ends first: touching ranges do not overlap.
	return x->index < y->index ? -1 : x->index > y->index;
}

typedef struct s_piece { uint64_t start, end; uint32_t index; } s_piece;

// Returns pieces (malloc) and their count. `starts`/`ends`/`orders` describe the n ranges.
static s_piece* s_sweep(const uint64_t* starts, const uint64_t* ends, const uint32_t* orders, size_t n, size_t* out_count)
{
	s_event* ev = (s_event*)malloc((n * 2 + 1) * sizeof(s_event));
	uint32_t* active = (uint32_t*)malloc((n + 1) * sizeof(uint32_t));
	s_piece* pieces = (s_piece*)malloc((n * 2 + 1) * sizeof(s_piece));
	size_t ne = 0, na = 0, np = 0, i = 0;
	uint64_t prev = 0;
	*out_count = 0;
	if (!ev || !active || !pieces) { free(ev); free(active); free(pieces); return NULL; }
	for (i = 0; i < n; ++i) {
		if (ends[i] <= starts[i]) continue;
		ev[ne].pos = starts[i]; ev[ne].index = (uint32_t)i; ev[ne].is_end = 0; ++ne;
		ev[ne].pos = ends[i]; ev[ne].index = (uint32_t)i; ev[ne].is_end = 1; ++ne;
	}
	qsort(ev, ne, sizeof(s_event), s_event_cmp);
	for (i = 0; i < ne; ) {
		uint64_t pos = ev[i].pos;
		if (na > 0 && pos > prev) {
			size_t k, best = active[0];
			for (k = 1; k < na; ++k) if (orders[active[k]] < orders[best]) best = active[k];
			if (np > 0 && pieces[np - 1].index == best && pieces[np - 1].end == prev) pieces[np - 1].end = pos;
			else { pieces[np].start = prev; pieces[np].end = pos; pieces[np].index = (uint32_t)best; ++np; }
		}
		for (; i < ne && ev[i].pos == pos; ++i) {
			if (ev[i].is_end) {
				size_t k;
				for (k = 0; k < na; ++k) if (active[k] == ev[i].index) { active[k] = active[na - 1]; --na; break; }
			} else {
				active[na++] = ev[i].index;
			}
		}
		prev = pos;
	}
	free(ev); free(active);
	*out_count = np;
	return pieces;
}

static int s_inl_cmp(const void* a, const void* b)
{
	const s_inl_rec* x = (const s_inl_rec*)a; const s_inl_rec* y = (const s_inl_rec*)b;
	if (x->start != y->start) return x->start < y->start ? -1 : 1;
	if (x->depth != y->depth) return x->depth < y->depth ? -1 : 1;
	if (x->end != y->end) return x->end > y->end ? -1 : 1;
	return x->order < y->order ? -1 : x->order > y->order;
}

static int s_u32_cmp(const void* a, const void* b)
{
	uint32_t x = *(const uint32_t*)a, y = *(const uint32_t*)b;
	return x < y ? -1 : x > y;
}

static size_t s_align8(size_t n) { return (n + 7) & ~(size_t)7; }

// Finishes the sink into an owned table. The sink is consumed either way.
static bool s_sink_finish(sym_sink* s, sym_table* out, sym_log_fn log, void* udata)
{
	uint64_t* starts = NULL; uint64_t* ends = NULL; uint32_t* orders = NULL;
	s_piece* fpieces = NULL; size_t fcount = 0;
	s_piece* lpieces = NULL; size_t lcount = 0;
	uint32_t* files = NULL; size_t file_count = 0, i, k;
	uint32_t* stack = NULL; size_t depth = 0;
	size_t nmax, off_funcs, off_lines, off_inlines, off_files, off_strings, total;
	unsigned char* bytes = NULL;
	bool ok = false;
	memset(out, 0, sizeof(*out));
	if (s->failed) { sym_logf(log, udata, "cute_sym: out of memory while building"); goto done; }
	sym_sink_line_end(s, 0);
	nmax = s->func_count > s->line_count ? s->func_count : s->line_count;
	starts = (uint64_t*)malloc((nmax + 1) * sizeof(uint64_t));
	ends = (uint64_t*)malloc((nmax + 1) * sizeof(uint64_t));
	orders = (uint32_t*)malloc((nmax + 1) * sizeof(uint32_t));
	if (!starts || !ends || !orders) goto done;

	for (i = 0; i < s->func_count; ++i) { starts[i] = s->funcs[i].start; ends[i] = s->funcs[i].end; orders[i] = s->funcs[i].order; }
	fpieces = s_sweep(starts, ends, orders, s->func_count, &fcount);
	if (s->func_count && !fpieces) goto done;
	// Pieces of different functions with the same name and touching ranges merge.
	for (i = 0, k = 0; i < fcount; ++i) {
		if (k > 0 && fpieces[k - 1].end == fpieces[i].start && s->funcs[fpieces[k - 1].index].name == s->funcs[fpieces[i].index].name) fpieces[k - 1].end = fpieces[i].end;
		else fpieces[k++] = fpieces[i];
	}
	fcount = k;

	for (i = 0; i < s->line_count; ++i) { starts[i] = s->lines[i].start; ends[i] = s->lines[i].end; orders[i] = s->lines[i].order; }
	lpieces = s_sweep(starts, ends, orders, s->line_count, &lcount);
	if (s->line_count && !lpieces) goto done;
	for (i = 0, k = 0; i < lcount; ++i) {
		const s_line_rec* a = k ? &s->lines[lpieces[k - 1].index] : NULL;
		const s_line_rec* b = &s->lines[lpieces[i].index];
		if (a && lpieces[k - 1].end == lpieces[i].start && a->file == b->file && a->line == b->line) lpieces[k - 1].end = lpieces[i].end;
		else lpieces[k++] = lpieces[i];
	}
	lcount = k;

	// Inline parents: a stack of enclosing ranges in (start, depth) order.
	qsort(s->inls, s->inl_count, sizeof(s_inl_rec), s_inl_cmp);
	stack = (uint32_t*)malloc((s->inl_count + 1) * sizeof(uint32_t));
	if (!stack) goto done;
	for (i = 0; i < s->inl_count; ++i) {
		s_inl_rec* r = &s->inls[i];
		while (depth > 0 && !(s->inls[stack[depth - 1]].start <= r->start && s->inls[stack[depth - 1]].end >= r->end)) --depth;
		r->parent = SYM_NO_PARENT;
		if (r->depth > 0) {
			size_t d = depth;
			while (d > 0) { --d; if (s->inls[stack[d]].depth == r->depth - 1) { r->parent = stack[d]; break; } }
		}
		stack[depth++] = (uint32_t)i;
	}

	// Distinct file strings, by offset.
	files = (uint32_t*)malloc((lcount + s->inl_count + 1) * sizeof(uint32_t));
	if (!files) goto done;
	for (i = 0; i < lcount; ++i) files[file_count++] = s->lines[lpieces[i].index].file;
	for (i = 0; i < s->inl_count; ++i) files[file_count++] = s->inls[i].call_file;
	qsort(files, file_count, sizeof(uint32_t), s_u32_cmp);
	for (i = 0, k = 0; i < file_count; ++i) if (files[i] && (k == 0 || files[k - 1] != files[i])) files[k++] = files[i];
	file_count = k;

	off_funcs = SYM_HEADER_BYTES;
	off_lines = s_align8(off_funcs + fcount * sizeof(sym_func));
	off_inlines = s_align8(off_lines + lcount * sizeof(sym_line));
	off_files = s_align8(off_inlines + s->inl_count * sizeof(sym_inline));
	off_strings = s_align8(off_files + file_count * 4);
	total = s_align8(off_strings + s->strs_len);
	if (total > 0xFFFFFFFFu) { sym_logf(log, udata, "cute_sym: table over 4 GB"); goto done; }
	bytes = (unsigned char*)calloc(1, total);
	if (!bytes) goto done;
	memcpy(bytes, "CUTESYM\0", 8);
	s_wr32(bytes + 8, SYM_VERSION);
	s_wr32(bytes + 12, (uint32_t)s->arch);
	memcpy(bytes + 16, s->build_id, 20);
	bytes[36] = (unsigned char)s->build_id_len;
	bytes[37] = (unsigned char)((lcount ? 1 : 0) | (s->inl_count ? 2 : 0));
	s_wr32(bytes + 40, (uint32_t)fcount);
	s_wr32(bytes + 44, (uint32_t)lcount);
	s_wr32(bytes + 48, (uint32_t)s->inl_count);
	s_wr32(bytes + 52, (uint32_t)file_count);
	s_wr32(bytes + 56, (uint32_t)s->strs_len);
	s_wr32(bytes + 60, (uint32_t)off_funcs);
	s_wr32(bytes + 64, (uint32_t)off_lines);
	s_wr32(bytes + 68, (uint32_t)off_inlines);
	s_wr32(bytes + 72, (uint32_t)off_files);
	s_wr32(bytes + 76, (uint32_t)off_strings);
	s_wr32(bytes + 80, (uint32_t)total);
	for (i = 0; i < fcount; ++i) {
		unsigned char* p = bytes + off_funcs + i * sizeof(sym_func);
		uint64_t size = fpieces[i].end - fpieces[i].start;
		s_wr64(p, fpieces[i].start);
		s_wr32(p + 8, size > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)size);
		s_wr32(p + 12, s->funcs[fpieces[i].index].name);
	}
	for (i = 0; i < lcount; ++i) {
		unsigned char* p = bytes + off_lines + i * sizeof(sym_line);
		const s_line_rec* r = &s->lines[lpieces[i].index];
		uint64_t size = lpieces[i].end - lpieces[i].start;
		s_wr64(p, lpieces[i].start);
		s_wr32(p + 8, size > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)size);
		s_wr32(p + 12, r->file);
		s_wr32(p + 16, r->line);
	}
	for (i = 0; i < s->inl_count; ++i) {
		unsigned char* p = bytes + off_inlines + i * sizeof(sym_inline);
		const s_inl_rec* r = &s->inls[i];
		uint64_t size = r->end - r->start;
		s_wr64(p, r->start);
		s_wr32(p + 8, size > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)size);
		s_wr32(p + 12, r->callee);
		s_wr32(p + 16, r->call_file);
		s_wr32(p + 20, r->call_line);
		s_wr32(p + 24, r->parent);
	}
	for (i = 0; i < file_count; ++i) s_wr32(bytes + off_files + i * 4, files[i]);
	memcpy(bytes + off_strings, s->strs, s->strs_len);
	ok = sym_read(bytes, total, out);
	if (ok) out->owned = true;
	else { sym_logf(log, udata, "cute_sym: the built table failed its own validation"); free(bytes); }
done:
	free(starts); free(ends); free(orders); free(fpieces); free(lpieces); free(files); free(stack);
	s_sink_free(s);
	return ok;
}

//--------------------------------------------------------------------------------------------------
// Reading, lookup, writing, embedding.

bool sym_read(const void* data, size_t len, sym_table* out)
{
	const unsigned char* b = (const unsigned char*)data;
	uint32_t total, off[5], counts[4], strings_len, i;
	memset(out, 0, sizeof(*out));
	if (!b || len < SYM_HEADER_BYTES || memcmp(b, "CUTESYM\0", 8) != 0) return false;
	if (s_rd32(b + 8) != SYM_VERSION) return false;
	total = s_rd32(b + 80);
	if (total > len || total < SYM_HEADER_BYTES) return false;
	counts[0] = s_rd32(b + 40); counts[1] = s_rd32(b + 44); counts[2] = s_rd32(b + 48); counts[3] = s_rd32(b + 52);
	strings_len = s_rd32(b + 56);
	for (i = 0; i < 5; ++i) off[i] = s_rd32(b + 60 + i * 4);
	// Every section lies inside total, is 8-aligned, and does not overflow.
	{
		uint64_t sizes[5] = { (uint64_t)counts[0] * 16, (uint64_t)counts[1] * 24, (uint64_t)counts[2] * 32, (uint64_t)counts[3] * 4, strings_len };
		for (i = 0; i < 5; ++i) {
			if (off[i] & 7) return false;
			if (off[i] < SYM_HEADER_BYTES || (uint64_t)off[i] + sizes[i] > total) return false;
		}
	}
	if (strings_len == 0 || b[off[4] + strings_len - 1] != 0 || b[off[4]] != 0) return false;
	if (b[36] > 20) return false;
	out->bytes = b; out->len = total;
	out->arch = (int)s_rd32(b + 12);
	memcpy(out->build_id, b + 16, 20);
	out->build_id_len = b[36];
	out->funcs = (const sym_func*)(b + off[0]); out->func_count = counts[0];
	out->lines = (const sym_line*)(b + off[1]); out->line_count = counts[1];
	out->inlines = (const sym_inline*)(b + off[2]); out->inline_count = counts[2];
	out->files = (const uint32_t*)(b + off[3]); out->file_count = counts[3];
	out->strings = (const char*)(b + off[4]); out->strings_len = strings_len;
	// Sorted, and every string offset in range.
	for (i = 0; i < counts[0]; ++i) {
		const unsigned char* p = b + off[0] + (size_t)i * 16;
		if (i && s_rd64(p) < s_rd64(p - 16) + s_rd32(p - 16 + 8)) return false;
		if (s_rd32(p + 12) >= strings_len) return false;
	}
	for (i = 0; i < counts[1]; ++i) {
		const unsigned char* p = b + off[1] + (size_t)i * 24;
		if (i && s_rd64(p) < s_rd64(p - 24) + s_rd32(p - 24 + 8)) return false;
		if (s_rd32(p + 12) >= strings_len) return false;
	}
	for (i = 0; i < counts[2]; ++i) {
		const unsigned char* p = b + off[2] + (size_t)i * 32;
		uint32_t parent = s_rd32(p + 24);
		if (i && s_rd64(p) < s_rd64(p - 32)) return false;
		if (s_rd32(p + 12) >= strings_len || s_rd32(p + 16) >= strings_len) return false;
		if (parent != SYM_NO_PARENT && parent >= i) return false;
	}
	for (i = 0; i < counts[3]; ++i) if (s_rd32(b + off[3] + (size_t)i * 4) >= strings_len) return false;
	return true;
}

bool sym_read_file(const char* path, sym_table* out)
{
	size_t len;
	unsigned char* bytes = s_read_file(path, &len);
	if (!bytes) { memset(out, 0, sizeof(*out)); return false; }
	if (!sym_read(bytes, len, out)) { free(bytes); return false; }
	out->owned = true;
	return true;
}

void sym_free(sym_table* t)
{
	if (t->owned) free((void*)t->bytes);
	memset(t, 0, sizeof(*t));
}

static const char* s_str(const sym_table* t, uint32_t off) { return off < t->strings_len ? t->strings + off : ""; }

// Index of the last record whose start <= addr, or -1. `stride` is the record size.
static long s_find(const unsigned char* base, uint32_t count, size_t stride, uint64_t addr)
{
	long lo = 0, hi = (long)count - 1, best = -1;
	while (lo <= hi) {
		long mid = lo + (hi - lo) / 2;
		if (s_rd64(base + (size_t)mid * stride) <= addr) { best = mid; lo = mid + 1; }
		else hi = mid - 1;
	}
	return best;
}

bool sym_lookup(const sym_table* t, uint64_t addr, sym_frame* out, sym_frame* inlined, int inlined_cap, int* inlined_count)
{
	const unsigned char* fb = (const unsigned char*)t->funcs;
	const unsigned char* lb = (const unsigned char*)t->lines;
	const unsigned char* ib = (const unsigned char*)t->inlines;
	long fi, li, ii;
	uint64_t fstart = 0;
	bool found = false;
	memset(out, 0, sizeof(*out));
	out->function = ""; out->file = "";
	if (inlined_count) *inlined_count = 0;
	fi = s_find(fb, t->func_count, 16, addr);
	if (fi >= 0 && addr < s_rd64(fb + fi * 16) + s_rd32(fb + fi * 16 + 8)) {
		out->function = s_str(t, s_rd32(fb + fi * 16 + 12));
		fstart = s_rd64(fb + fi * 16);
		found = true;
	}
	li = s_find(lb, t->line_count, 24, addr);
	if (li >= 0 && addr < s_rd64(lb + li * 24) + s_rd32(lb + li * 24 + 8)) {
		out->file = s_str(t, s_rd32(lb + li * 24 + 12));
		out->line = s_rd32(lb + li * 24 + 16);
		found = true;
	}
	// The innermost inline range is the containing one with the greatest start: walk back
	// from the last start <= addr, bounded by the function (or a fixed budget without one).
	ii = s_find(ib, t->inline_count, 32, addr);
	if (ii >= 0 && inlined && inlined_cap > 0) {
		long budget = fi >= 0 ? (long)t->inline_count : 4096;
		long k = ii, inner = -1;
		for (; k >= 0 && budget-- > 0; --k) {
			const unsigned char* p = ib + k * 32;
			uint64_t start = s_rd64(p);
			if (fi >= 0 && start < fstart) break;
			if (addr < start + s_rd32(p + 8)) { inner = k; break; }
		}
		if (inner >= 0) {
			int n = 0;
			uint32_t cur = (uint32_t)inner;
			while (cur != SYM_NO_PARENT && n < inlined_cap) {
				const unsigned char* p = ib + (size_t)cur * 32;
				inlined[n].function = s_str(t, s_rd32(p + 12));
				inlined[n].file = s_str(t, s_rd32(p + 16));
				inlined[n].line = s_rd32(p + 20);
				++n;
				cur = s_rd32(p + 24);
			}
			if (inlined_count) *inlined_count = n;
			found = true;
		}
	}
	return found;
}

bool sym_write(const sym_table* t, const char* path)
{
	return t->bytes && s_write_file(path, t->bytes, t->len);
}

static long s_find_slot(const unsigned char* file, size_t len, uint32_t* capacity)
{
	size_t i;
	for (i = 0; i + SYM_SLOT_HEAD <= len; ++i) {
		if (file[i] == 'C' && memcmp(file + i, SYM_SLOT_MAGIC, 12) == 0) {
			*capacity = s_rd32(file + i + 12);
			if (i + SYM_SLOT_HEAD + *capacity <= len) return (long)i;
		}
	}
	return -1;
}

bool sym_embed(const sym_table* t, const char* binary_path, sym_log_fn log, void* udata)
{
	size_t len;
	unsigned char* file = s_read_file(binary_path, &len);
	uint32_t capacity = 0;
	long at;
	bool ok;
	if (!file) { sym_logf(log, udata, "cute_sym: cannot read %s", binary_path); return false; }
	at = s_find_slot(file, len, &capacity);
	if (at < 0) { sym_logf(log, udata, "cute_sym: %s has no symbol slot (build it with CUTE_CRASH_SYM_RESERVE)", binary_path); free(file); return false; }
	if (t->len > capacity) {
		sym_logf(log, udata, "cute_sym: table is %u bytes, the slot in %s holds %u: raise CUTE_CRASH_SYM_RESERVE", (unsigned)t->len, binary_path, (unsigned)capacity);
		free(file);
		return false;
	}
	memset(file + at + SYM_SLOT_HEAD, 0, capacity);
	memcpy(file + at + SYM_SLOT_HEAD, t->bytes, t->len);
	ok = s_write_file(binary_path, file, len);
	if (!ok) sym_logf(log, udata, "cute_sym: cannot write %s", binary_path);
	free(file);
	return ok;
}

// A table found in a binary's slot (not owned: the caller keeps `file`).
static bool s_slot_table(const unsigned char* file, size_t len, sym_table* out)
{
	uint32_t capacity = 0;
	long at = s_find_slot(file, len, &capacity);
	if (at < 0) return false;
	return sym_read(file + at + SYM_SLOT_HEAD, capacity, out);
}

//--------------------------------------------------------------------------------------------------
// PE: headers, sections (for RVAs), the CodeView debug directory entry naming the PDB.

typedef struct s_pe_section { uint32_t rva, vsize, raw_off, raw_size; } s_pe_section;

typedef struct s_pe {
	const unsigned char* file; size_t len;
	int arch;
	uint32_t image_base_lo;
	s_pe_section sections[256]; int section_count;
	unsigned char guid[16]; uint32_t age; bool has_cv;
	char pdb_path[1024];
	uint32_t debug_rva, debug_size;
} s_pe;

static uint32_t s_pe_rva_to_off(const s_pe* pe, uint32_t rva)
{
	int i;
	for (i = 0; i < pe->section_count; ++i) {
		const s_pe_section* s = &pe->sections[i];
		uint32_t span = s->vsize > s->raw_size ? s->vsize : s->raw_size;
		if (rva >= s->rva && rva < s->rva + span) return rva - s->rva + s->raw_off;
	}
	return 0xFFFFFFFFu;
}

static bool s_pe_parse(s_pe* pe, const unsigned char* file, size_t len, sym_log_fn log, void* udata)
{
	uint32_t pe_off, opt_size, nsec, i, ddir_off, ndirs;
	uint16_t machine, magic;
	const unsigned char* fh;
	memset(pe, 0, sizeof(*pe));
	pe->file = file; pe->len = len;
	if (len < 0x40 || file[0] != 'M' || file[1] != 'Z') return false;
	pe_off = s_rd32(file + 0x3C);
	if ((size_t)pe_off + 24 > len || memcmp(file + pe_off, "PE\0\0", 4) != 0) return false;
	fh = file + pe_off + 4;
	machine = s_rd16(fh);
	nsec = s_rd16(fh + 2);
	opt_size = s_rd16(fh + 16);
	pe->arch = machine == 0x8664 ? SYM_ARCH_X86_64 : machine == 0xAA64 ? SYM_ARCH_AARCH64 : machine == 0x14C ? SYM_ARCH_X86 : SYM_ARCH_NONE;
	if ((size_t)pe_off + 24 + opt_size > len || opt_size < 96) return false;
	magic = s_rd16(fh + 20);
	if (magic == 0x20B) { ndirs = s_rd32(fh + 20 + 108); ddir_off = 20 + 112; }
	else if (magic == 0x10B) { ndirs = s_rd32(fh + 20 + 92); ddir_off = 20 + 96; }
	else return false;
	if (ndirs > 6 && (size_t)ddir_off + 7 * 8 <= 20u + opt_size) {
		pe->debug_rva = s_rd32(fh + ddir_off + 6 * 8);
		pe->debug_size = s_rd32(fh + ddir_off + 6 * 8 + 4);
	}
	if (nsec > 256) nsec = 256;
	for (i = 0; i < nsec; ++i) {
		const unsigned char* sh = file + pe_off + 24 + opt_size + i * 40;
		if ((size_t)(sh - file) + 40 > len) break;
		pe->sections[i].vsize = s_rd32(sh + 8);
		pe->sections[i].rva = s_rd32(sh + 12);
		pe->sections[i].raw_size = s_rd32(sh + 16);
		pe->sections[i].raw_off = s_rd32(sh + 20);
		pe->section_count = (int)i + 1;
	}
	if (pe->debug_rva && pe->debug_size >= 28) {
		uint32_t off = s_pe_rva_to_off(pe, pe->debug_rva);
		uint32_t n = pe->debug_size / 28, k;
		for (k = 0; off != 0xFFFFFFFFu && k < n; ++k) {
			const unsigned char* e = file + off + k * 28;
			uint32_t type, size, raw;
			if ((size_t)(e - file) + 28 > len) break;
			type = s_rd32(e + 12); size = s_rd32(e + 16); raw = s_rd32(e + 24);
			if (type == 2 && size >= 24 && (size_t)raw + size <= len && memcmp(file + raw, "RSDS", 4) == 0) {
				size_t plen = size - 24;
				memcpy(pe->guid, file + raw + 4, 16);
				pe->age = s_rd32(file + raw + 20);
				if (plen >= sizeof(pe->pdb_path)) plen = sizeof(pe->pdb_path) - 1;
				memcpy(pe->pdb_path, file + raw + 24, plen);
				pe->pdb_path[plen] = 0;
				pe->has_cv = true;
				break;
			}
		}
	}
	if (!pe->has_cv) sym_logf(log, udata, "cute_sym: no CodeView debug directory entry (built without a PDB?)");
	return true;
}

// The GUID in PRINTED order (Data1, Data2, Data3 big-endian, Data4 as is), as pools and tools show
// it, then the age big-endian: 20 bytes. cute_crash.h emits the same.
static void s_pe_build_id(const s_pe* pe, unsigned char out[20])
{
	const unsigned char* g = pe->guid;
	out[0] = g[3]; out[1] = g[2]; out[2] = g[1]; out[3] = g[0];
	out[4] = g[5]; out[5] = g[4];
	out[6] = g[7]; out[7] = g[6];
	memcpy(out + 8, g + 8, 8);
	out[16] = (unsigned char)(pe->age >> 24); out[17] = (unsigned char)(pe->age >> 16);
	out[18] = (unsigned char)(pe->age >> 8); out[19] = (unsigned char)pe->age;
}

//--------------------------------------------------------------------------------------------------
// PDB. The MSF container holds numbered streams; the ones that matter: 1 (PDB info: GUID, age,
// the named stream map that names "/names"), 2 (TPI: class names for member inlinees), 3 (DBI:
// the module list), 4 (IPI: inlinee function ids), "/names" (file names), each module's stream
// (CodeView symbols with procedures and inline sites; C13 line and file-checksum subsections),
// and the symbol record stream (publics, for code no module describes).

typedef struct s_msf {
	const unsigned char* file; size_t len;
	uint32_t block_size, num_blocks;
	unsigned char* dir; size_t dir_len;
	uint32_t stream_count;
} s_msf;

static bool s_msf_open(s_msf* m, const unsigned char* file, size_t len)
{
	static const char magic[] = "Microsoft C/C++ MSF 7.00\r\n\x1a" "DS\0\0";
	uint32_t dir_bytes, map_block, nblocks, i;
	memset(m, 0, sizeof(*m));
	if (len < 64 || memcmp(file, magic, 32) != 0) return false;
	m->file = file; m->len = len;
	m->block_size = s_rd32(file + 32);
	m->num_blocks = s_rd32(file + 40);
	dir_bytes = s_rd32(file + 44);
	map_block = s_rd32(file + 52);
	if (m->block_size < 512 || m->block_size > 65536 || (m->block_size & (m->block_size - 1))) return false;
	if ((uint64_t)map_block * m->block_size + 4 > len) return false;
	nblocks = (dir_bytes + m->block_size - 1) / m->block_size;
	if ((uint64_t)map_block * m->block_size + (uint64_t)nblocks * 4 > len) return false;
	m->dir = (unsigned char*)malloc((size_t)nblocks * m->block_size + 4);
	if (!m->dir) return false;
	for (i = 0; i < nblocks; ++i) {
		uint32_t blk = s_rd32(file + (size_t)map_block * m->block_size + i * 4);
		if ((uint64_t)blk * m->block_size + m->block_size > len) { free(m->dir); m->dir = NULL; return false; }
		memcpy(m->dir + (size_t)i * m->block_size, file + (size_t)blk * m->block_size, m->block_size);
	}
	m->dir_len = dir_bytes;
	if (dir_bytes < 4) { free(m->dir); m->dir = NULL; return false; }
	m->stream_count = s_rd32(m->dir);
	if ((uint64_t)4 + (uint64_t)m->stream_count * 4 > dir_bytes) { free(m->dir); m->dir = NULL; return false; }
	return true;
}

static void s_msf_close(s_msf* m) { free(m->dir); m->dir = NULL; }

// A stream copied out contiguously (malloc), or NULL when absent or malformed.
static unsigned char* s_msf_stream(const s_msf* m, uint32_t index, size_t* out_len)
{
	uint32_t i, size, nblocks, pos = 4 + m->stream_count * 4;
	unsigned char* out;
	*out_len = 0;
	if (index >= m->stream_count) return NULL;
	for (i = 0; i < index; ++i) {
		uint32_t sz = s_rd32(m->dir + 4 + i * 4);
		if (sz != 0xFFFFFFFFu) pos += ((sz + m->block_size - 1) / m->block_size) * 4;
	}
	size = s_rd32(m->dir + 4 + index * 4);
	if (size == 0xFFFFFFFFu || size == 0) return NULL;
	nblocks = (size + m->block_size - 1) / m->block_size;
	if ((uint64_t)pos + (uint64_t)nblocks * 4 > m->dir_len) return NULL;
	out = (unsigned char*)malloc((size_t)nblocks * m->block_size + 8);
	if (!out) return NULL;
	for (i = 0; i < nblocks; ++i) {
		uint32_t blk = s_rd32(m->dir + pos + i * 4);
		if ((uint64_t)blk * m->block_size + m->block_size > m->len) { free(out); return NULL; }
		memcpy(out + (size_t)i * m->block_size, m->file + (size_t)blk * m->block_size, m->block_size);
	}
	memset(out + size, 0, 8);
	*out_len = size;
	return out;
}

// A type/id stream (TPI or IPI): record offsets by index, so a lookup is one array read.
typedef struct s_tpi { unsigned char* bytes; size_t len; uint32_t begin, end; uint32_t* offsets; } s_tpi;

static bool s_tpi_open(s_tpi* t, const s_msf* m, uint32_t stream)
{
	uint32_t hdr, pos, i;
	memset(t, 0, sizeof(*t));
	t->bytes = s_msf_stream(m, stream, &t->len);
	if (!t->bytes || t->len < 56) { free(t->bytes); t->bytes = NULL; return false; }
	hdr = s_rd32(t->bytes + 4);
	t->begin = s_rd32(t->bytes + 8);
	t->end = s_rd32(t->bytes + 12);
	if (hdr > t->len || t->end < t->begin || t->end - t->begin > 50000000u) { free(t->bytes); t->bytes = NULL; return false; }
	t->offsets = (uint32_t*)malloc(((size_t)(t->end - t->begin) + 1) * 4);
	if (!t->offsets) { free(t->bytes); t->bytes = NULL; return false; }
	pos = hdr;
	for (i = 0; i < t->end - t->begin; ++i) {
		uint16_t reclen;
		if ((size_t)pos + 4 > t->len) break;
		reclen = s_rd16(t->bytes + pos);
		t->offsets[i] = pos;
		pos += 2u + reclen;
	}
	for (; i < t->end - t->begin; ++i) t->offsets[i] = 0xFFFFFFFFu;
	return true;
}

static void s_tpi_close(s_tpi* t) { free(t->bytes); free(t->offsets); memset(t, 0, sizeof(*t)); }

// The record for an index: kind and payload, or false.
static bool s_tpi_record(const s_tpi* t, uint32_t index, uint16_t* kind, const unsigned char** payload, size_t* payload_len)
{
	uint32_t off;
	uint16_t reclen;
	if (!t->bytes || index < t->begin || index >= t->end) return false;
	off = t->offsets[index - t->begin];
	if (off == 0xFFFFFFFFu || (size_t)off + 4 > t->len) return false;
	reclen = s_rd16(t->bytes + off);
	if (reclen < 2 || (size_t)off + 2 + reclen > t->len) return false;
	*kind = s_rd16(t->bytes + off + 2);
	*payload = t->bytes + off + 4;
	*payload_len = reclen - 2u;
	return true;
}

// Skips a CodeView numeric leaf; returns the bytes it occupied, or 0 when unknown.
static size_t s_cv_numeric_len(const unsigned char* p, size_t len)
{
	uint16_t v;
	if (len < 2) return 0;
	v = s_rd16(p);
	if (v < 0x8000) return 2;
	switch (v) {
	case 0x8000: return 3;
	case 0x8001: case 0x8002: return 4;
	case 0x8003: case 0x8004: case 0x8005: return 6;
	case 0x8009: case 0x800A: return 10;
	default: return 0;
	}
}

// The name of a TPI class/struct/union/enum record, or NULL.
static const char* s_tpi_type_name(const s_tpi* tpi, uint32_t index)
{
	uint16_t kind;
	const unsigned char* p;
	size_t n, skip;
	if (!s_tpi_record(tpi, index, &kind, &p, &n)) return NULL;
	switch (kind) {
	case 0x1504: case 0x1505: case 0x1519: // LF_CLASS LF_STRUCTURE LF_INTERFACE
		if (n < 18) return NULL;
		skip = s_cv_numeric_len(p + 16, n - 16);
		return skip ? (const char*)p + 16 + skip : NULL;
	case 0x1506: // LF_UNION
		if (n < 10) return NULL;
		skip = s_cv_numeric_len(p + 8, n - 8);
		return skip ? (const char*)p + 8 + skip : NULL;
	case 0x1507: // LF_ENUM
		return n > 12 ? (const char*)p + 12 : NULL;
	default: return NULL;
	}
}

// The qualified name of an IPI function id (LF_FUNC_ID: scope string + name; LF_MFUNC_ID: class
// name + name), into `out`.
static bool s_ipi_func_name(const s_tpi* ipi, const s_tpi* tpi, uint32_t id, char* out, size_t cap)
{
	uint16_t kind;
	const unsigned char* p;
	size_t n;
	const char* scope = NULL;
	const char* name;
	if (!s_tpi_record(ipi, id, &kind, &p, &n) || n < 9) return false;
	name = (const char*)p + 8;
	if (kind == 0x1601) { // LF_FUNC_ID
		uint32_t scope_id = s_rd32(p);
		if (scope_id) {
			uint16_t sk; const unsigned char* sp; size_t sn;
			if (s_tpi_record(ipi, scope_id, &sk, &sp, &sn) && sk == 0x1605 && sn > 4) scope = (const char*)sp + 4;
		}
	} else if (kind == 0x1602) { // LF_MFUNC_ID
		scope = s_tpi_type_name(tpi, s_rd32(p));
	} else {
		return false;
	}
	if (scope && scope[0]) snprintf(out, cap, "%s::%s", scope, name);
	else snprintf(out, cap, "%s", name);
	out[cap - 1] = 0;
	return true;
}

// Per module, the C13 pieces: file checksums (names by their byte offset in the subsection),
// line rows (RVA, length, file, line) sorted by RVA, and inlinee base lines by item id.
typedef struct s_cv_row { uint32_t rva, len, file, line; } s_cv_row; // file: /names offset
typedef struct s_cv_inlinee { uint32_t id, file, line; } s_cv_inlinee;

typedef struct s_cv_module {
	const unsigned char* c13; size_t c13_len;
	const unsigned char* chksums; size_t chksums_len; // The FILECHKSMS subsection body.
	s_cv_row* rows; size_t row_count, row_cap;
	s_cv_inlinee* inlinees; size_t inlinee_count, inlinee_cap;
} s_cv_module;

// A record of an inline site: a run of the inlinee's code at a line.
typedef struct s_cv_rec { uint32_t rva, len, file, line; } s_cv_rec;
typedef struct s_cv_piece { uint32_t start, end; } s_cv_piece;
typedef struct s_cv_site { int depth, parent; uint32_t inlinee; s_cv_rec* recs; size_t rec_count, rec_cap; s_cv_piece* pieces; size_t piece_count, piece_cap; } s_cv_site;

static int s_cv_row_cmp(const void* a, const void* b)
{
	uint32_t x = ((const s_cv_row*)a)->rva, y = ((const s_cv_row*)b)->rva;
	return x < y ? -1 : x > y;
}

typedef struct s_pdb {
	const s_pe* pe;
	s_msf msf;
	unsigned char* names; size_t names_len; // "/names": strings start at 12.
	s_tpi tpi, ipi;
	sym_sink* sink;
	sym_log_fn log; void* udata;
	uint32_t* proc_starts; uint32_t* proc_ends; size_t proc_count, proc_cap; // Every procedure, for sizing and vetting publics.
} s_pdb;

static uint32_t s_pdb_rva(const s_pdb* p, uint16_t seg, uint32_t off)
{
	if (seg == 0 || seg > p->pe->section_count) return 0;
	return p->pe->sections[seg - 1].rva + off;
}

static const char* s_pdb_name_at(const s_pdb* p, uint32_t off)
{
	if (!p->names || (size_t)12 + off >= p->names_len) return "";
	return (const char*)p->names + 12 + off;
}

// The file name behind a checksum-table offset.
static const char* s_cv_file(const s_pdb* p, const s_cv_module* m, uint32_t chk_off)
{
	if (!m->chksums || (size_t)chk_off + 4 > m->chksums_len) return "";
	return s_pdb_name_at(p, s_rd32(m->chksums + chk_off));
}

static void s_cv_parse_c13(s_pdb* p, s_cv_module* m)
{
	size_t pos = 0;
	while (pos + 8 <= m->c13_len) {
		uint32_t kind = s_rd32(m->c13 + pos), len = s_rd32(m->c13 + pos + 4);
		const unsigned char* body = m->c13 + pos + 8;
		if (len > m->c13_len - pos - 8) break;
		if (kind == 0xF4) { m->chksums = body; m->chksums_len = len; }
		pos += 8 + ((len + 3) & ~3u);
	}
	pos = 0;
	while (pos + 8 <= m->c13_len) {
		uint32_t kind = s_rd32(m->c13 + pos), len = s_rd32(m->c13 + pos + 4);
		const unsigned char* body = m->c13 + pos + 8;
		if (len > m->c13_len - pos - 8) break;
		if (kind == 0xF2 && len >= 12) {
			uint32_t base = s_pdb_rva(p, s_rd16(body + 4), s_rd32(body));
			uint16_t flags = s_rd16(body + 6);
			uint32_t code_size = s_rd32(body + 8);
			size_t bpos = 12;
			while (bpos + 12 <= len) {
				uint32_t file = s_rd32(body + bpos), nlines = s_rd32(body + bpos + 4), block = s_rd32(body + bpos + 8);
				size_t need = 12 + (size_t)nlines * 8 + ((flags & 1) ? (size_t)nlines * 4 : 0), i;
				if (block < need || bpos + block > len) break;
				// Rows sharing an offset (a function's declaration and its opening brace at
				// offset 0): the first owns that one address, the last owns the rest, which is
				// how dbghelp reads them.
				for (i = 0; i < nlines; ++i) {
					const unsigned char* l = body + bpos + 12 + i * 8;
					uint32_t off = s_rd32(l), v = s_rd32(l + 4);
					uint32_t next = code_size, last_v = v;
					size_t j, last = i;
					s_cv_row* r;
					// The statement bit is not a filter: clang-cl clears it on every row, dbghelp
					// reads them all the same.
					for (j = i + 1; j < nlines; ++j) {
						uint32_t o = s_rd32(body + bpos + 12 + j * 8), vj = s_rd32(body + bpos + 12 + j * 8 + 4);
						if (o > off) { next = o; break; }
						last = j; last_v = vj;
					}
					if (next < off) next = off;
					m->rows = (s_cv_row*)s_grow(m->rows, &m->row_cap, m->row_count + 1, sizeof(s_cv_row));
					if (!m->rows) { p->sink->failed = true; return; }
					if (last != i && next > off + 1) {
						r = &m->rows[m->row_count++];
						r->rva = base + off; r->len = 1; r->file = file; r->line = v & 0xFFFFFF;
						r = &m->rows[m->row_count++];
						r->rva = base + off + 1; r->len = next - off - 1; r->file = file; r->line = last_v & 0xFFFFFF;
					} else {
						r = &m->rows[m->row_count++];
						r->rva = base + off; r->len = next - off; r->file = file; r->line = (last != i ? last_v : v) & 0xFFFFFF;
					}
					i = last;
				}
				bpos += block;
			}
		} else if (kind == 0xF6 && len >= 4) {
			uint32_t sig = s_rd32(body);
			size_t ipos = 4;
			while (ipos + 12 <= len) {
				s_cv_inlinee* e;
				uint32_t extra = 0;
				if (sig == 1) { if (ipos + 16 > len) break; extra = s_rd32(body + ipos + 12); }
				m->inlinees = (s_cv_inlinee*)s_grow(m->inlinees, &m->inlinee_cap, m->inlinee_count, sizeof(s_cv_inlinee));
				if (!m->inlinees) { p->sink->failed = true; return; }
				e = &m->inlinees[m->inlinee_count++];
				e->id = s_rd32(body + ipos); e->file = s_rd32(body + ipos + 4); e->line = s_rd32(body + ipos + 8);
				ipos += sig == 1 ? 16 + (size_t)extra * 4 : 12;
			}
		}
		pos += 8 + ((len + 3) & ~3u);
	}
	qsort(m->rows, m->row_count, sizeof(s_cv_row), s_cv_row_cmp);
}

static const s_cv_inlinee* s_cv_inlinee_of(const s_cv_module* m, uint32_t id)
{
	size_t i;
	for (i = 0; i < m->inlinee_count; ++i) if (m->inlinees[i].id == id) return &m->inlinees[i];
	return NULL;
}

// The module's statement row covering an RVA, or NULL.
static const s_cv_row* s_cv_row_at(const s_cv_module* m, uint32_t rva)
{
	long lo = 0, hi = (long)m->row_count - 1, best = -1;
	while (lo <= hi) {
		long mid = lo + (hi - lo) / 2;
		if (m->rows[mid].rva <= rva) { best = mid; lo = mid + 1; } else hi = mid - 1;
	}
	if (best < 0) return NULL;
	return rva < m->rows[best].rva + m->rows[best].len ? &m->rows[best] : NULL;
}

// Binary annotations: compressed operands, 0xFF terminates.
static bool s_cv_annot_u(const unsigned char** pp, const unsigned char* end, uint32_t* out)
{
	const unsigned char* p = *pp;
	unsigned char b;
	if (p >= end) return false;
	b = *p;
	if ((b & 0x80) == 0) { *out = b; *pp = p + 1; return true; }
	if ((b & 0xC0) == 0x80) { if (p + 2 > end) return false; *out = ((uint32_t)(b & 0x3F) << 8) | p[1]; *pp = p + 2; return true; }
	if ((b & 0xE0) == 0xC0) { if (p + 4 > end) return false; *out = ((uint32_t)(b & 0x1F) << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; *pp = p + 4; return true; }
	return false;
}

static int32_t s_cv_annot_s(uint32_t u) { return (u & 1) ? -(int32_t)(u >> 1) : (int32_t)(u >> 1); }

static void s_cv_site_rec(s_cv_site* s, uint32_t rva, uint32_t len, uint32_t file, uint32_t line, bool* failed)
{
	s_cv_rec* r;
	s->recs = (s_cv_rec*)s_grow(s->recs, &s->rec_cap, s->rec_count, sizeof(s_cv_rec));
	if (!s->recs) { *failed = true; return; }
	r = &s->recs[s->rec_count++];
	r->rva = rva; r->len = len; r->file = file; r->line = line;
}

// Decodes one site's annotations into records. A record opens whenever the code offset moves
// and holds the line and file in force then; its length is explicit (ChangeCodeLength, or the
// first operand of ChangeCodeLengthAndCodeOffset) or runs to the next record. Only the offset
// operands move the running offset; a length never does (MSVC's emission, checked against
// llvm-symbolizer).
static void s_cv_decode_site(s_cv_site* s, const unsigned char* p, const unsigned char* end, uint32_t func_rva, const s_cv_inlinee* base, bool* failed)
{
	uint32_t offset = 0, file = base ? base->file : 0;
	int32_t line = base ? (int32_t)base->line : 0;
	bool open = false;
	uint32_t open_rva = 0, open_file = 0; int32_t open_line = 0;
	while (p < end) {
		uint32_t op, u1 = 0, u2 = 0;
		bool moved = false;
		if (!s_cv_annot_u(&p, end, &op) || op == 0) break;
		switch (op) {
		case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: case 13:
			if (!s_cv_annot_u(&p, end, &u1)) return;
			break;
		case 11:
			if (!s_cv_annot_u(&p, end, &u1)) return;
			break;
		case 12:
			if (!s_cv_annot_u(&p, end, &u1) || !s_cv_annot_u(&p, end, &u2)) return;
			break;
		default:
			return;
		}
		switch (op) {
		case 1: offset = u1; moved = true; break;                          // CodeOffset
		case 2: break;                                                     // ChangeCodeOffsetBase: separated code; unsupported
		case 3: offset += u1; moved = true; break;                         // ChangeCodeOffset
		case 4:                                                            // ChangeCodeLength: the open record's length
			if (open) { s_cv_site_rec(s, open_rva, u1, open_file, (uint32_t)(open_line < 0 ? 0 : open_line), failed); open = false; }
			break;
		case 5: file = u1; break;                                          // ChangeFile
		case 6: line += s_cv_annot_s(u1); break;                           // ChangeLineOffset
		case 7: case 8: case 9: case 10: case 13: break;                   // Line end, range kind, columns: not kept
		case 11: offset += u1 & 0xF; line += s_cv_annot_s(u1 >> 4); moved = true; break; // ChangeCodeOffsetAndLineOffset
		case 12:                                                           // ChangeCodeLengthAndCodeOffset
			if (open) { s_cv_site_rec(s, open_rva, func_rva + offset + u2 - open_rva, open_file, (uint32_t)(open_line < 0 ? 0 : open_line), failed); open = false; }
			offset += u2;
			s_cv_site_rec(s, func_rva + offset, u1, file, (uint32_t)(line < 0 ? 0 : line), failed);
			break;
		default: break;
		}
		if (moved) {
			if (open) { s_cv_site_rec(s, open_rva, func_rva + offset - open_rva, open_file, (uint32_t)(open_line < 0 ? 0 : open_line), failed); }
			open = true; open_rva = func_rva + offset; open_file = file; open_line = line;
		}
	}
	if (open) s_cv_site_rec(s, open_rva, 0, open_file, (uint32_t)(open_line < 0 ? 0 : open_line), failed); // Runs to the next boundary.
}

// The line in force at an RVA in the enclosing scope (the procedure's statement rows, or a
// parent site's records) and where that line's run ends, or false.
static bool s_cv_context_at(const s_cv_module* m, const s_cv_site* sites, int parent, uint32_t rva, uint32_t* file, uint32_t* line, uint32_t* end)
{
	if (parent < 0) {
		const s_cv_row* row = s_cv_row_at(m, rva);
		if (!row) return false;
		*file = row->file; *line = row->line; *end = row->rva + row->len;
		return true;
	} else {
		const s_cv_site* s = &sites[parent];
		size_t i;
		for (i = 0; i < s->rec_count; ++i) {
			const s_cv_rec* r = &s->recs[i];
			uint32_t rend = r->len ? r->rva + r->len : (i + 1 < s->rec_count ? s->recs[i + 1].rva : 0xFFFFFFFFu);
			if (rva >= r->rva && rva < rend) { *file = r->file; *line = r->line; *end = rend; return true; }
		}
		return false;
	}
}

// Emits one procedure: its inline sites' rows and ranges deepest first, so they win the sweep
// over the procedure's own rows, which name the call site at inlined code.
static void s_cv_emit_proc(s_pdb* p, s_cv_module* m, uint32_t proc_rva, uint32_t proc_len, const char* proc_name, s_cv_site* sites, size_t site_count)
{
	size_t i;
	int max_depth = 0;
	bool* failed = &p->sink->failed;
	char name[4096];
	sym_sink_function(p->sink, proc_rva, proc_len, proc_name);
	if (p->proc_count >= p->proc_cap) {
		size_t ncap = p->proc_cap ? p->proc_cap * 2 : 1024;
		uint32_t* ns = (uint32_t*)realloc(p->proc_starts, ncap * sizeof(uint32_t));
		uint32_t* ne = (uint32_t*)realloc(p->proc_ends, ncap * sizeof(uint32_t));
		if (ns) p->proc_starts = ns;
		if (ne) p->proc_ends = ne;
		if (!ns || !ne) { *failed = true; return; }
		p->proc_cap = ncap;
	}
	p->proc_starts[p->proc_count] = proc_rva;
	p->proc_ends[p->proc_count] = proc_rva + proc_len;
	p->proc_count++;
	for (i = 0; i < site_count; ++i) if (sites[i].depth > max_depth) max_depth = sites[i].depth;
	// Close open tails at the next statement row or sibling record, never past the procedure.
	for (i = 0; i < site_count; ++i) {
		s_cv_site* s = &sites[i];
		size_t k;
		for (k = 0; k < s->rec_count; ++k) {
			s_cv_rec* r = &s->recs[k];
			if (r->len) continue;
			if (k + 1 < s->rec_count && s->recs[k + 1].rva > r->rva) { r->len = s->recs[k + 1].rva - r->rva; continue; }
			{
				uint32_t endv = proc_rva + proc_len;
				const s_cv_row* row = s_cv_row_at(m, r->rva);
				if (row && row->rva + row->len > r->rva && row->rva + row->len < endv) endv = row->rva + row->len;
				r->len = endv > r->rva ? endv - r->rva : 0;
			}
		}
	}
	// The inline ranges, parents first: contiguous runs of a site's records, split wherever the
	// enclosing scope's line changes and at the parent's own piece boundaries, so each range
	// names one call site and lies inside one parent range.
	{
		int d;
		for (d = 0; d <= max_depth; ++d) {
			for (i = 0; i < site_count; ++i) {
				s_cv_site* s = &sites[i];
				size_t run_start;
				if (s->depth != d || s->rec_count == 0) continue;
				if (!s_ipi_func_name(&p->ipi, &p->tpi, s->inlinee, name, sizeof(name))) snprintf(name, sizeof(name), "inlinee#%u", (unsigned)s->inlinee);
				for (run_start = 0; run_start < s->rec_count; ) {
					size_t run_end = run_start;
					uint32_t start = s->recs[run_start].rva, endv = start, pos;
					while (run_end < s->rec_count && s->recs[run_end].rva <= endv) {
						if (s->recs[run_end].rva + s->recs[run_end].len > endv) endv = s->recs[run_end].rva + s->recs[run_end].len;
						++run_end;
					}
					for (pos = start; pos < endv; ) {
						uint32_t call_file = 0, call_line = 0, ctx_end = endv, piece_end = endv;
						bool have_call = s_cv_context_at(m, sites, s->parent, pos, &call_file, &call_line, &ctx_end);
						s_cv_piece* piece;
						if (have_call && ctx_end > pos && ctx_end < piece_end) piece_end = ctx_end;
						if (s->parent >= 0) {
							const s_cv_site* ps = &sites[s->parent];
							size_t k;
							uint32_t bound = 0;
							for (k = 0; k < ps->piece_count; ++k) if (pos >= ps->pieces[k].start && pos < ps->pieces[k].end) { bound = ps->pieces[k].end; break; }
							if (bound == 0) { // Outside every parent piece: no parent, up to the next one.
								have_call = false;
								for (k = 0; k < ps->piece_count; ++k) if (ps->pieces[k].start > pos && ps->pieces[k].start < piece_end) piece_end = ps->pieces[k].start;
							} else if (bound < piece_end) {
								piece_end = bound;
							}
						}
						s->pieces = (s_cv_piece*)s_grow(s->pieces, &s->piece_cap, s->piece_count, sizeof(s_cv_piece));
						if (!s->pieces) { *failed = true; return; }
						piece = &s->pieces[s->piece_count++];
						piece->start = pos; piece->end = piece_end;
						sym_sink_inline(p->sink, pos, piece_end - pos, name, have_call ? s_cv_file(p, m, call_file) : "", have_call ? call_line : 0, s->depth);
						pos = piece_end;
					}
					run_start = run_end > run_start ? run_end : run_start + 1;
				}
			}
		}
	}
	// The inlinees' own rows, deepest first, so the innermost code names its own line.
	for (; max_depth >= 0; --max_depth) {
		for (i = 0; i < site_count; ++i) {
			const s_cv_site* s = &sites[i];
			size_t k;
			if (s->depth != max_depth) continue;
			for (k = 0; k < s->rec_count; ++k) {
				const s_cv_rec* r = &s->recs[k];
				if (r->len) sym_sink_line(p->sink, r->rva, r->len, s_cv_file(p, m, r->file), r->line);
			}
		}
	}
}

static void s_cv_module_symbols(s_pdb* p, s_cv_module* m, const unsigned char* syms, size_t len)
{
	size_t pos = 0;
	// The current procedure and its sites; a stack of open scopes by site index (-1 = not a site).
	bool in_proc = false;
	uint32_t proc_rva = 0, proc_len = 0;
	char proc_name[4096];
	s_cv_site* sites = NULL; size_t site_count = 0, site_cap = 0;
	int stack[256]; int depth = 0;
	proc_name[0] = 0;
	while (pos + 4 <= len && !p->sink->failed) {
		uint16_t reclen = s_rd16(syms + pos), kind = s_rd16(syms + pos + 2);
		const unsigned char* body = syms + pos + 4;
		size_t n = reclen >= 2 ? reclen - 2u : 0;
		if (reclen < 2 || pos + 2 + reclen > len) break;
		switch (kind) {
		case 0x110F: case 0x1110: case 0x1146: case 0x1147: case 0x1155: case 0x1156: // S_LPROC32 S_GPROC32 and _ID/_DPC variants
			if (n >= 36) {
				size_t nl = strnlen((const char*)body + 35, n - 35);
				if (nl >= sizeof(proc_name)) nl = sizeof(proc_name) - 1;
				memcpy(proc_name, body + 35, nl); proc_name[nl] = 0;
				proc_rva = s_pdb_rva(p, s_rd16(body + 32), s_rd32(body + 28));
				proc_len = s_rd32(body + 12);
				in_proc = true; site_count = 0; depth = 0;
				if (depth < 256) stack[depth++] = -1;
			}
			break;
		case 0x1103: // S_BLOCK32
			if (depth < 256) stack[depth++] = -1;
			break;
		case 0x114D: case 0x115D: // S_INLINESITE S_INLINESITE2
			if (in_proc && n >= 12) {
				s_cv_site* s;
				int parent = -1, k;
				size_t ann = kind == 0x114D ? 12 : 16;
				sites = (s_cv_site*)s_grow(sites, &site_cap, site_count, sizeof(s_cv_site));
				if (!sites) { p->sink->failed = true; break; }
				s = &sites[site_count];
				memset(s, 0, sizeof(*s));
				s->inlinee = s_rd32(body + 8);
				for (k = depth - 1; k >= 0; --k) if (stack[k] >= 0) { parent = stack[k]; break; }
				s->parent = parent;
				s->depth = parent >= 0 ? sites[parent].depth + 1 : 0;
				if (n > ann) s_cv_decode_site(s, body + ann, body + n, proc_rva, s_cv_inlinee_of(m, s->inlinee), &p->sink->failed);
				if (depth < 256) stack[depth++] = (int)site_count;
				site_count++;
			}
			break;
		case 0x0006: case 0x114E: case 0x114F: // S_END S_INLINESITE_END S_PROC_ID_END
			if (depth > 0) --depth;
			if (in_proc && depth == 0) {
				size_t i;
				s_cv_emit_proc(p, m, proc_rva, proc_len, proc_name, sites, site_count);
				for (i = 0; i < site_count; ++i) { free(sites[i].recs); free(sites[i].pieces); }
				site_count = 0;
				in_proc = false;
			}
			break;
		default: break;
		}
		pos += 2u + reclen;
	}
	{
		size_t i;
		for (i = 0; i < site_count; ++i) { free(sites[i].recs); free(sites[i].pieces); }
		free(sites);
	}
}

static int s_u32_cmp_ptr(const void* a, const void* b)
{
	uint32_t x = *(const uint32_t*)a, y = *(const uint32_t*)b;
	return x < y ? -1 : x > y;
}

static int s_u64_cmp_ptr(const void* a, const void* b)
{
	uint64_t x = *(const uint64_t*)a, y = *(const uint64_t*)b;
	return x < y ? -1 : x > y;
}

// Publics with the function flag, sized to the next known start in their section, pushed last
// so they only fill gaps.
static void s_pdb_publics(s_pdb* p, uint32_t sym_stream)
{
	size_t len, pos = 0, count = 0, cap = 0, i;
	unsigned char* syms = s_msf_stream(&p->msf, sym_stream, &len);
	typedef struct s_pub { uint32_t rva; const unsigned char* name; } s_pub;
	s_pub* pubs = NULL;
	uint32_t* starts;
	if (!syms) return;
	while (pos + 4 <= len) {
		uint16_t reclen = s_rd16(syms + pos), kind = s_rd16(syms + pos + 2);
		if (reclen < 2 || pos + 2 + reclen > len) break;
		if (kind == 0x110E && reclen >= 14 && (s_rd32(syms + pos + 4) & 2)) {
			s_pub* e;
			pubs = (s_pub*)s_grow(pubs, &cap, count, sizeof(s_pub));
			if (!pubs) break;
			e = &pubs[count++];
			e->rva = s_pdb_rva(p, s_rd16(syms + pos + 12), s_rd32(syms + pos + 8));
			e->name = syms + pos + 14;
		}
		pos += 2u + reclen;
	}
	starts = (uint32_t*)malloc((count + p->proc_count + 1) * sizeof(uint32_t));
	if (starts && pubs) {
		size_t n = 0, nprocs = p->proc_count;
		uint64_t* ranges = (uint64_t*)malloc((nprocs + 1) * sizeof(uint64_t)); // start << 32 | end, sorted.
		for (i = 0; i < count; ++i) starts[n++] = pubs[i].rva;
		for (i = 0; i < p->proc_count; ++i) starts[n++] = p->proc_starts[i];
		qsort(starts, n, sizeof(uint32_t), s_u32_cmp_ptr);
		if (ranges) {
			for (i = 0; i < nprocs; ++i) ranges[i] = ((uint64_t)p->proc_starts[i] << 32) | p->proc_ends[i];
			qsort(ranges, nprocs, sizeof(uint64_t), s_u64_cmp_ptr);
		}
		for (i = 0; i < count; ++i) {
			uint32_t rva = pubs[i].rva, next = 0, sec_end = 0;
			int k;
			long lo = 0, hi = (long)n - 1, first_after = -1;
			if (ranges) { // Inside a procedure already named by its module: not a gap.
				long rlo = 0, rhi = (long)nprocs - 1, last = -1;
				while (rlo <= rhi) { long mid = rlo + (rhi - rlo) / 2; if ((uint32_t)(ranges[mid] >> 32) <= rva) { last = mid; rlo = mid + 1; } else rhi = mid - 1; }
				if (last >= 0 && rva < (uint32_t)ranges[last]) continue;
			}
			for (k = 0; k < p->pe->section_count; ++k) {
				const s_pe_section* s = &p->pe->sections[k];
				if (rva >= s->rva && rva < s->rva + (s->vsize ? s->vsize : s->raw_size)) { sec_end = s->rva + (s->vsize ? s->vsize : s->raw_size); break; }
			}
			if (!sec_end || !rva) continue;
			while (lo <= hi) { long mid = lo + (hi - lo) / 2; if (starts[mid] > rva) { first_after = mid; hi = mid - 1; } else lo = mid + 1; }
			next = first_after >= 0 && starts[first_after] < sec_end ? starts[first_after] : sec_end;
			if (next > rva) sym_sink_function(p->sink, rva, next - rva, (const char*)pubs[i].name);
		}
		free(ranges);
	}
	free(starts); free(pubs); free(syms);
}

// Statement rows of a module are pushed after the module's procedures, so a procedure's inline
// rows take precedence. s_cv_module_symbols pushes per procedure; the rows go here, per module.
static void s_cv_module_rows(s_pdb* p, const s_cv_module* m)
{
	size_t i;
	for (i = 0; i < m->row_count; ++i) {
		const s_cv_row* r = &m->rows[i];
		if (r->len) sym_sink_line(p->sink, r->rva, r->len, s_cv_file(p, m, r->file), r->line);
	}
}

static bool s_pdb_build_from(s_pdb* p, const unsigned char* file, size_t len)
{
	unsigned char* info = NULL; size_t info_len = 0;
	unsigned char* dbi = NULL; size_t dbi_len = 0;
	uint32_t names_stream = 0xFFFFFFFFu;
	bool ok = false;
	if (!s_msf_open(&p->msf, file, len)) { sym_logf(p->log, p->udata, "cute_sym: not an MSF 7.00 PDB"); return false; }
	info = s_msf_stream(&p->msf, 1, &info_len);
	if (!info || info_len < 28) { sym_logf(p->log, p->udata, "cute_sym: PDB info stream missing"); goto done; }
	if (memcmp(info + 12, p->pe->guid, 16) != 0) {
		char a[40], b[40];
		s_hex(info + 12, 16, a); s_hex(p->pe->guid, 16, b);
		sym_logf(p->log, p->udata, "cute_sym: PDB GUID %s does not match the binary's %s", a, b);
		goto done;
	}
	if (s_rd32(info + 8) != p->pe->age) sym_logf(p->log, p->udata, "cute_sym: PDB age %u differs from the binary's %u (incremental link?)", (unsigned)s_rd32(info + 8), (unsigned)p->pe->age);
	// The named stream map: find "/names".
	{
		uint32_t nlen = s_rd32(info + 28), size, cap, present_words, deleted_words, i, pos;
		const char* names = (const char*)info + 32;
		if ((size_t)32 + nlen + 8 > info_len) goto nonames;
		pos = 32 + nlen;
		size = s_rd32(info + pos); cap = s_rd32(info + pos + 4); pos += 8;
		(void)cap;
		if ((size_t)pos + 4 > info_len) goto nonames;
		present_words = s_rd32(info + pos); pos += 4 + present_words * 4;
		if ((size_t)pos + 4 > info_len) goto nonames;
		deleted_words = s_rd32(info + pos); pos += 4 + deleted_words * 4;
		for (i = 0; i < size && (size_t)pos + 8 <= info_len; ++i, pos += 8) {
			uint32_t key = s_rd32(info + pos), value = s_rd32(info + pos + 4);
			if (key < nlen && strcmp(names + key, "/names") == 0) { names_stream = value; break; }
		}
	}
nonames:
	if (names_stream != 0xFFFFFFFFu) {
		p->names = s_msf_stream(&p->msf, names_stream, &p->names_len);
		if (p->names && (p->names_len < 12 || s_rd32(p->names) != 0xEFFEEFFEu)) { free(p->names); p->names = NULL; p->names_len = 0; }
	}
	if (!p->names) sym_logf(p->log, p->udata, "cute_sym: PDB has no /names stream; file names will be empty");
	s_tpi_open(&p->tpi, &p->msf, 2);
	s_tpi_open(&p->ipi, &p->msf, 4);
	dbi = s_msf_stream(&p->msf, 3, &dbi_len);
	if (!dbi || dbi_len < 64) { sym_logf(p->log, p->udata, "cute_sym: PDB DBI stream missing"); goto done; }
	{
		uint16_t sym_stream = s_rd16(dbi + 20);
		int32_t mod_size = (int32_t)s_rd32(dbi + 24);
		size_t pos = 64, end = mod_size > 0 ? 64 + (size_t)mod_size : 64;
		int modules = 0, with_symbols = 0;
		if (end > dbi_len) end = dbi_len;
		while (pos + 64 <= end) {
			uint16_t stream = s_rd16(dbi + pos + 34);
			uint32_t sym_bytes = s_rd32(dbi + pos + 36), c11_bytes = s_rd32(dbi + pos + 40), c13_bytes = s_rd32(dbi + pos + 44);
			size_t name_pos = pos + 64, nl1, nl2;
			nl1 = strnlen((const char*)dbi + name_pos, end - name_pos);
			nl2 = name_pos + nl1 + 1 < end ? strnlen((const char*)dbi + name_pos + nl1 + 1, end - name_pos - nl1 - 1) : 0;
			pos = (name_pos + nl1 + 1 + nl2 + 1 + 3) & ~(size_t)3;
			modules++;
			if (stream != 0xFFFF) {
				size_t mlen;
				unsigned char* mod = s_msf_stream(&p->msf, stream, &mlen);
				if (mod) {
					s_cv_module m;
					memset(&m, 0, sizeof(m));
					if ((size_t)sym_bytes + c11_bytes + c13_bytes <= mlen) {
						m.c13 = mod + sym_bytes + c11_bytes; m.c13_len = c13_bytes;
						s_cv_parse_c13(p, &m);
						if (sym_bytes > 4) s_cv_module_symbols(p, &m, mod + 4, sym_bytes - 4);
						s_cv_module_rows(p, &m);
						with_symbols++;
					}
					free(m.rows); free(m.inlinees);
					free(mod);
				}
			}
		}
		if (with_symbols == 0) sym_logf(p->log, p->udata, "cute_sym: PDB has no module symbols (stripped: /PDBSTRIPPED): functions from publics only, no lines");
		// The procedures' own rows were pushed per module after their inline rows; the publics last.
		if (sym_stream != 0xFFFF) s_pdb_publics(p, sym_stream);
		ok = !p->sink->failed;
	}
done:
	free(info); free(dbi);
	free(p->names); p->names = NULL;
	s_tpi_close(&p->tpi); s_tpi_close(&p->ipi);
	s_msf_close(&p->msf);
	free(p->proc_starts); free(p->proc_ends); p->proc_starts = p->proc_ends = NULL;
	return ok;
}


static bool s_pdb_build(const s_pe* pe, const char* binary_path, const char* debug_path, sym_sink* sink, sym_log_fn log, void* udata)
{
	char path[2048], dir[1024];
	size_t len = 0;
	unsigned char* file = NULL;
	s_pdb p;
	bool ok;
	memset(&p, 0, sizeof(p));
	p.pe = pe; p.sink = sink; p.log = log; p.udata = udata;
	if (debug_path) file = s_read_file(debug_path, &len);
	if (!file && pe->pdb_path[0]) file = s_read_file(pe->pdb_path, &len);
	s_dirname(binary_path, dir, sizeof(dir));
	if (!file && pe->pdb_path[0]) { snprintf(path, sizeof(path), "%s/%s", dir, s_basename(pe->pdb_path)); file = s_read_file(path, &len); }
	if (!file) {
		const char* b = s_basename(binary_path);
		const char* dot = strrchr(b, '.');
		size_t stem = dot ? (size_t)(dot - b) : strlen(b);
		snprintf(path, sizeof(path), "%s/%.*s.pdb", dir, (int)stem, b);
		file = s_read_file(path, &len);
	}
	if (!file) { sym_logf(log, udata, "cute_sym: no PDB for %s (looked for %s)", binary_path, pe->pdb_path[0] ? pe->pdb_path : path); return false; }
	ok = s_pdb_build_from(&p, file, len);
	free(file);
	return ok;
}

//--------------------------------------------------------------------------------------------------
// The DWARF side: ELF and Mach-O containers (fat, dSYM, the debug map and its object files) and
// the DWARF 2-5 reader behind them, plus DWARF carried inside a PE.

bool sym_build_elf(const unsigned char* file, size_t len, const char* path, sym_sink* sink, sym_log_fn log, void* udata);
bool sym_build_macho(const unsigned char* file, size_t len, const char* path, const char* debug_path, sym_sink* sink, sym_log_fn log, void* udata);
bool sym_build_pe_dwarf(const unsigned char* file, size_t len, sym_sink* sink, sym_log_fn log, void* udata);

/*
	cute_sym_dwarf.inc -- the DWARF half of cute_sym.h: the ELF and Mach-O containers and the DWARF
	reader behind sym_build_elf, sym_build_macho and sym_build_pe_dwarf. This is a fragment of
	cute_sym.h's implementation, included after the sink API, not a header of its own: no guard,
	nothing visible outside but the three entry points.

	Every byte is read through a bounds-checked cursor, so a truncated or hostile file ends in a
	logged failure, never a fault. Addresses handed to the sink follow the convention of the table:
	ELF link-time vaddr, Mach-O link-time vmaddr, PE RVA. A Mach-O executable without its own DWARF
	is read through its debug map: each object file's DWARF, relocations applied, rebased function by
	function onto the linked addresses the map records.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ---------------------------------------------------------------------------------------------- */
/* The cursor. Every read checks; a failed read marks the cursor and yields zero. */

typedef struct s_dw_cur { const unsigned char* p; const unsigned char* end; int ok; } s_dw_cur;

static s_dw_cur s_dw_cur_make(const unsigned char* p, size_t len)
{
	s_dw_cur c;
	c.p = p;
	c.end = p ? p + len : NULL;
	c.ok = p != NULL;
	return c;
}

static int s_dw_left(const s_dw_cur* c, size_t n) { return c->ok && (size_t)(c->end - c->p) >= n; }

static uint8_t s_dw_u8(s_dw_cur* c)
{
	if (!s_dw_left(c, 1)) { c->ok = 0; return 0; }
	return *c->p++;
}

static uint16_t s_dw_u16(s_dw_cur* c)
{
	if (!s_dw_left(c, 2)) { c->ok = 0; return 0; }
	uint16_t v = (uint16_t)(c->p[0] | (c->p[1] << 8));
	c->p += 2;
	return v;
}

static uint32_t s_dw_u32(s_dw_cur* c)
{
	if (!s_dw_left(c, 4)) { c->ok = 0; return 0; }
	uint32_t v = (uint32_t)c->p[0] | ((uint32_t)c->p[1] << 8) | ((uint32_t)c->p[2] << 16) | ((uint32_t)c->p[3] << 24);
	c->p += 4;
	return v;
}

static uint64_t s_dw_u64(s_dw_cur* c)
{
	uint64_t lo = s_dw_u32(c);
	uint64_t hi = s_dw_u32(c);
	return lo | (hi << 32);
}

static uint32_t s_dw_u32be(s_dw_cur* c)
{
	if (!s_dw_left(c, 4)) { c->ok = 0; return 0; }
	uint32_t v = ((uint32_t)c->p[0] << 24) | ((uint32_t)c->p[1] << 16) | ((uint32_t)c->p[2] << 8) | (uint32_t)c->p[3];
	c->p += 4;
	return v;
}

static uint64_t s_dw_u64be(s_dw_cur* c)
{
	uint64_t hi = s_dw_u32be(c);
	uint64_t lo = s_dw_u32be(c);
	return (hi << 32) | lo;
}

static uint64_t s_dw_sized(s_dw_cur* c, int size)
{
	switch (size) {
	case 1: return s_dw_u8(c);
	case 2: return s_dw_u16(c);
	case 4: return s_dw_u32(c);
	case 8: return s_dw_u64(c);
	default: c->ok = 0; return 0;
	}
}

static uint64_t s_dw_uleb(s_dw_cur* c)
{
	uint64_t r = 0;
	int shift = 0;
	for (;;) {
		uint8_t b = s_dw_u8(c);
		if (!c->ok) return 0;
		if (shift < 64) r |= (uint64_t)(b & 0x7f) << shift;
		shift += 7;
		if (!(b & 0x80)) break;
		if (shift > 70) { c->ok = 0; return 0; }
	}
	return r;
}

static int64_t s_dw_sleb(s_dw_cur* c)
{
	int64_t r = 0;
	int shift = 0;
	uint8_t b = 0;
	for (;;) {
		b = s_dw_u8(c);
		if (!c->ok) return 0;
		if (shift < 64) r |= (int64_t)((uint64_t)(b & 0x7f) << shift);
		shift += 7;
		if (!(b & 0x80)) break;
		if (shift > 70) { c->ok = 0; return 0; }
	}
	if (shift < 64 && (b & 0x40)) r |= -((int64_t)1 << shift);
	return r;
}

static void s_dw_skip(s_dw_cur* c, uint64_t n)
{
	if (!c->ok || n > (uint64_t)(c->end - c->p)) { c->ok = 0; return; }
	c->p += n;
}

/* Returns the string and steps past its NUL; NULL when no NUL lies within the cursor. */
static const char* s_dw_cstr(s_dw_cur* c)
{
	if (!c->ok) return NULL;
	const unsigned char* q = c->p;
	while (q < c->end && *q) ++q;
	if (q >= c->end) { c->ok = 0; return NULL; }
	const char* s = (const char*)c->p;
	c->p = q + 1;
	return s;
}

/* ---------------------------------------------------------------------------------------------- */
/* Sections and small helpers. */

typedef struct s_dw_sec { const unsigned char* data; size_t len; } s_dw_sec;

typedef struct s_dw_sections
{
	s_dw_sec info, abbrev, str, line, line_str, str_offsets, addr, ranges, rnglists, aranges;
	s_dw_sec types; /* DWARF 4 type units (-fdebug-types-section); DWARF 5 keeps them in info */
} s_dw_sections;

/* A function translating addresses as they appear in the DWARF into the table's convention. NULL
   fn is the identity. Returning 0 drops the address (an object-file range the map does not cover).
   `span_end` is the translated end of the contiguous span the address lies in: line rows of one
   DWARF sequence may land in different places once rebased, so a sequence is closed at each span's
   end before the next span's rows begin. Identity translations report no end (~0). */
typedef struct s_dw_rebase { int (*fn)(void* ctx, uint64_t addr, uint64_t* out, uint64_t* span_end); void* ctx; } s_dw_rebase;

static const char* s_dw_str_at(const s_dw_sec* s, uint64_t off)
{
	if (!s->data || off >= s->len) return NULL;
	const unsigned char* p = s->data + off;
	const unsigned char* end = s->data + s->len;
	while (p < end && *p) ++p;
	if (p >= end) return NULL;
	return (const char*)(s->data + off);
}

static int s_dw_grow(void** items, int* cap, int need, size_t elem)
{
	if (need <= *cap) return 1;
	int ncap = *cap ? *cap * 2 : 64;
	while (ncap < need) ncap *= 2;
	void* n = realloc(*items, (size_t)ncap * elem);
	if (!n) return 0;
	*items = n;
	*cap = ncap;
	return 1;
}

static char* s_dw_strdup(const char* s)
{
	size_t n = strlen(s) + 1;
	char* d = (char*)malloc(n);
	if (d) memcpy(d, s, n);
	return d;
}

static unsigned char* s_dw_read_file(const char* path, size_t* len)
{
	FILE* f = fopen(path, "rb");
	if (!f) return NULL;
	if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
	long n = ftell(f);
	if (n < 0) { fclose(f); return NULL; }
	if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
	unsigned char* buf = (unsigned char*)malloc((size_t)n + 1);
	if (!buf) { fclose(f); return NULL; }
	size_t got = fread(buf, 1, (size_t)n, f);
	fclose(f);
	if (got != (size_t)n) { free(buf); return NULL; }
	buf[n] = 0;
	*len = (size_t)n;
	return buf;
}

/* Joins dir and name into out, keeping separators as written and dropping a leading "./". */
static void s_dw_path_join(char* out, size_t cap, const char* comp_dir, const char* dir, const char* name)
{
	out[0] = 0;
	if (!name) return;
	int name_abs = name[0] == '/' || name[0] == '\\' || (name[0] && name[1] == ':');
	int dir_abs = dir && (dir[0] == '/' || dir[0] == '\\' || (dir[0] && dir[1] == ':'));
	size_t n = 0;
	if (!name_abs) {
		if (!dir_abs && comp_dir && comp_dir[0]) {
			n = strlen(comp_dir);
			if (n >= cap) n = cap - 1;
			memcpy(out, comp_dir, n);
			if (n && out[n - 1] != '/' && out[n - 1] != '\\' && n + 1 < cap) out[n++] = '/';
		}
		if (dir && dir[0] && !(dir[0] == '.' && dir[1] == 0)) {
			size_t d = strlen(dir);
			if (d >= cap - n) d = cap - n - 1;
			memcpy(out + n, dir, d);
			n += d;
			if (n && out[n - 1] != '/' && out[n - 1] != '\\' && n + 1 < cap) out[n++] = '/';
		}
	}
	while (name[0] == '.' && (name[1] == '/' || name[1] == '\\')) name += 2;
	size_t m = strlen(name);
	if (m >= cap - n) m = cap - n - 1;
	memcpy(out + n, name, m);
	out[n + m] = 0;
}

/* ---------------------------------------------------------------------------------------------- */
/* DWARF constants: only what the reader needs. */

#define S_DW_TAG_class_type         0x02
#define S_DW_TAG_enumeration_type   0x04
#define S_DW_TAG_lexical_block      0x0b
#define S_DW_TAG_compile_unit       0x11
#define S_DW_TAG_structure_type     0x13
#define S_DW_TAG_union_type         0x17
#define S_DW_TAG_inlined_subroutine 0x1d
#define S_DW_TAG_subprogram         0x2e
#define S_DW_TAG_namespace          0x39
#define S_DW_TAG_partial_unit       0x3c

#define S_DW_AT_name             0x03
#define S_DW_AT_stmt_list        0x10
#define S_DW_AT_low_pc           0x11
#define S_DW_AT_high_pc          0x12
#define S_DW_AT_comp_dir         0x1b
#define S_DW_AT_inline           0x20
#define S_DW_AT_abstract_origin  0x31
#define S_DW_AT_declaration      0x3c
#define S_DW_AT_specification    0x47
#define S_DW_AT_entry_pc         0x52
#define S_DW_AT_ranges           0x55
#define S_DW_AT_call_file        0x58
#define S_DW_AT_call_line        0x59
#define S_DW_AT_linkage_name     0x6e
#define S_DW_AT_str_offsets_base 0x72
#define S_DW_AT_addr_base        0x73
#define S_DW_AT_rnglists_base    0x74
#define S_DW_AT_MIPS_linkage_name 0x2007

#define S_DW_FORM_addr           0x01
#define S_DW_FORM_block2         0x03
#define S_DW_FORM_block4         0x04
#define S_DW_FORM_data2          0x05
#define S_DW_FORM_data4          0x06
#define S_DW_FORM_data8          0x07
#define S_DW_FORM_string         0x08
#define S_DW_FORM_block          0x09
#define S_DW_FORM_block1         0x0a
#define S_DW_FORM_data1          0x0b
#define S_DW_FORM_flag           0x0c
#define S_DW_FORM_sdata          0x0d
#define S_DW_FORM_strp           0x0e
#define S_DW_FORM_udata          0x0f
#define S_DW_FORM_ref_addr       0x10
#define S_DW_FORM_ref1           0x11
#define S_DW_FORM_ref2           0x12
#define S_DW_FORM_ref4           0x13
#define S_DW_FORM_ref8           0x14
#define S_DW_FORM_ref_udata      0x15
#define S_DW_FORM_indirect       0x16
#define S_DW_FORM_sec_offset     0x17
#define S_DW_FORM_exprloc        0x18
#define S_DW_FORM_flag_present   0x19
#define S_DW_FORM_strx           0x1a
#define S_DW_FORM_addrx          0x1b
#define S_DW_FORM_ref_sup4       0x1c
#define S_DW_FORM_strp_sup       0x1d
#define S_DW_FORM_data16         0x1e
#define S_DW_FORM_line_strp      0x1f
#define S_DW_FORM_ref_sig8       0x20
#define S_DW_FORM_implicit_const 0x21
#define S_DW_FORM_loclistx       0x22
#define S_DW_FORM_rnglistx       0x23
#define S_DW_FORM_ref_sup8       0x24
#define S_DW_FORM_strx1          0x25
#define S_DW_FORM_strx2          0x26
#define S_DW_FORM_strx3          0x27
#define S_DW_FORM_strx4          0x28
#define S_DW_FORM_addrx1         0x29
#define S_DW_FORM_addrx2         0x2a
#define S_DW_FORM_addrx3         0x2b
#define S_DW_FORM_addrx4         0x2c
#define S_DW_FORM_GNU_addr_index 0x1f01
#define S_DW_FORM_GNU_str_index  0x1f02
#define S_DW_FORM_GNU_ref_alt    0x1f20
#define S_DW_FORM_GNU_strp_alt   0x1f21

#define S_DW_UT_compile       1
#define S_DW_UT_type          2
#define S_DW_UT_partial       3
#define S_DW_UT_skeleton      4
#define S_DW_UT_split_compile 5
#define S_DW_UT_split_type    6

#define S_DW_MAX_DEPTH 64
#define S_DW_MAX_WARNINGS 24

/* ---------------------------------------------------------------------------------------------- */
/* Units, abbreviations, DIEs. */

typedef struct s_dw_attr { uint16_t name; uint16_t form; int64_t implicit_const; } s_dw_attr;

typedef struct s_dw_abbrev { uint64_t code; uint16_t tag; uint8_t has_children; uint32_t attr_start; uint32_t attr_count; } s_dw_abbrev;

typedef struct s_dw_abbrevs
{
	s_dw_abbrev* items; int count, cap;
	s_dw_attr* attrs; int attr_count, attr_cap;
	int dense; /* codes are 1..count in order: index by code - 1 */
} s_dw_abbrevs;

typedef struct s_dw_die
{
	uint64_t offset;
	int32_t parent;   /* index, or -1 */
	int32_t abbrev;   /* index into abbrevs.items */
	uint16_t tag;
	uint8_t has_children;
	uint8_t depth;
} s_dw_die;

typedef struct s_dw_cu
{
	const s_dw_sec* sec; /* the section the unit lives in: info, or types for DWARF 4 type units */
	uint64_t offset, end, die_offset;
	uint64_t abbrev_offset;
	uint64_t signature, type_offset; /* type units: the signature scopes refer to, and the type DIE */
	uint16_t version;
	uint8_t unit_type, addr_size, dwarf64, offset_size, is_type;
	/* The unit DIE's attributes. */
	uint64_t low_pc; int has_low_pc;
	uint64_t str_offsets_base; int has_str_offsets_base;
	uint64_t addr_base; int has_addr_base;
	uint64_t rnglists_base; int has_rnglists_base;
	uint64_t stmt_list; int has_stmt_list;
	const char* name;
	const char* comp_dir;
	/* Loaded state. */
	int loaded;
	s_dw_abbrevs abbrevs;
	s_dw_die* dies; int die_count, die_cap;
	char** files; int file_count, file_cap; /* index per the unit's line table numbering */
	int files_done;
	int use; /* load sequence for the cache */
} s_dw_cu;

typedef struct s_dw_ctx
{
	const s_dw_sections* secs;
	const s_dw_rebase* rb;
	sym_sink* sink;
	sym_log_fn log;
	void* udata;
	s_dw_cu* cus; int cu_count, cu_cap;
	s_dw_cu* tus; int tu_count, tu_cap; /* type units, found by signature */
	int warnings;
	int use_seq;
	int current;
} s_dw_ctx;

static void s_dw_warn(s_dw_ctx* ctx, const char* fmt, const char* a, uint64_t b)
{
	if (ctx->warnings >= S_DW_MAX_WARNINGS) return;
	ctx->warnings++;
	if (ctx->warnings == S_DW_MAX_WARNINGS) {
		sym_logf(ctx->log, ctx->udata, "dwarf: further warnings suppressed");
		return;
	}
	sym_logf(ctx->log, ctx->udata, fmt, a ? a : "", (unsigned long long)b);
}

static int s_dw_rebase_addr(s_dw_ctx* ctx, uint64_t addr, uint64_t* out, uint64_t* span_end)
{
	uint64_t end = ~(uint64_t)0;
	int ok;
	if (!ctx->rb || !ctx->rb->fn) { *out = addr; ok = 1; }
	else ok = ctx->rb->fn(ctx->rb->ctx, addr, out, &end);
	if (span_end) *span_end = end;
	return ok;
}

/* Reads a unit header at `off` in `sec`; false at the end of the section or on damage. `types`
   says the section is DWARF 4's .debug_types, whose units carry a signature after the header. */
static int s_dw_unit_header(const s_dw_sec* sec, int types, uint64_t off, s_dw_cu* cu, int* skip)
{
	if (off >= sec->len) return 0;
	s_dw_cur c = s_dw_cur_make(sec->data + off, sec->len - (size_t)off);
	memset(cu, 0, sizeof(*cu));
	cu->sec = sec;
	cu->offset = off;
	uint64_t length = s_dw_u32(&c);
	if (length == 0xffffffffu) { length = s_dw_u64(&c); cu->dwarf64 = 1; cu->offset_size = 8; }
	else cu->offset_size = 4;
	if (!c.ok || length == 0) return 0;
	uint64_t after_length = (uint64_t)(c.p - sec->data);
	if (length > sec->len - after_length) return 0;
	cu->end = after_length + length;
	cu->version = s_dw_u16(&c);
	*skip = 0;
	if (cu->version >= 5) {
		cu->unit_type = s_dw_u8(&c);
		cu->addr_size = s_dw_u8(&c);
		cu->abbrev_offset = s_dw_sized(&c, cu->offset_size);
		if (cu->unit_type == S_DW_UT_type) { cu->signature = s_dw_u64(&c); cu->type_offset = s_dw_sized(&c, cu->offset_size); cu->is_type = 1; }
		else if (cu->unit_type == S_DW_UT_split_type) { s_dw_skip(&c, 8 + cu->offset_size); *skip = 1; }
		else if (cu->unit_type == S_DW_UT_skeleton || cu->unit_type == S_DW_UT_split_compile) { s_dw_skip(&c, 8); *skip = 1; }
	} else if (cu->version >= 2) {
		cu->unit_type = S_DW_UT_compile;
		cu->abbrev_offset = s_dw_sized(&c, cu->offset_size);
		cu->addr_size = s_dw_u8(&c);
		if (types) { cu->signature = s_dw_u64(&c); cu->type_offset = s_dw_sized(&c, cu->offset_size); cu->is_type = 1; }
	} else {
		return 0;
	}
	if (!c.ok) return 0;
	if (cu->addr_size != 4 && cu->addr_size != 8) *skip = 1;
	cu->die_offset = (uint64_t)(c.p - sec->data);
	return 1;
}

static int s_dw_enumerate_section(s_dw_ctx* ctx, const s_dw_sec* sec, int types)
{
	uint64_t off = 0;
	while (sec->data) {
		s_dw_cu cu;
		int skip = 0;
		if (!s_dw_unit_header(sec, types, off, &cu, &skip)) break;
		if (!skip) {
			if (cu.is_type) {
				if (!s_dw_grow((void**)&ctx->tus, &ctx->tu_cap, ctx->tu_count + 1, sizeof(s_dw_cu))) return 0;
				ctx->tus[ctx->tu_count++] = cu;
			} else {
				if (!s_dw_grow((void**)&ctx->cus, &ctx->cu_cap, ctx->cu_count + 1, sizeof(s_dw_cu))) return 0;
				ctx->cus[ctx->cu_count++] = cu;
			}
		}
		if (cu.end <= off) break;
		off = cu.end;
	}
	return 1;
}

static int s_dw_enumerate_units(s_dw_ctx* ctx)
{
	return s_dw_enumerate_section(ctx, &ctx->secs->info, 0) && s_dw_enumerate_section(ctx, &ctx->secs->types, 1);
}

static int s_dw_abbrevs_load(s_dw_ctx* ctx, s_dw_cu* cu)
{
	const s_dw_sec* sec = &ctx->secs->abbrev;
	if (cu->abbrev_offset >= sec->len) return 0;
	s_dw_cur c = s_dw_cur_make(sec->data + cu->abbrev_offset, sec->len - (size_t)cu->abbrev_offset);
	s_dw_abbrevs* ab = &cu->abbrevs;
	ab->dense = 1;
	for (;;) {
		uint64_t code = s_dw_uleb(&c);
		if (!c.ok) return 0;
		if (code == 0) break;
		s_dw_abbrev a;
		a.code = code;
		a.tag = (uint16_t)s_dw_uleb(&c);
		a.has_children = s_dw_u8(&c);
		a.attr_start = (uint32_t)ab->attr_count;
		a.attr_count = 0;
		for (;;) {
			uint64_t name = s_dw_uleb(&c);
			uint64_t form = s_dw_uleb(&c);
			if (!c.ok) return 0;
			if (name == 0 && form == 0) break;
			s_dw_attr at;
			at.name = (uint16_t)name;
			at.form = (uint16_t)form;
			at.implicit_const = form == S_DW_FORM_implicit_const ? s_dw_sleb(&c) : 0;
			if (!s_dw_grow((void**)&ab->attrs, &ab->attr_cap, ab->attr_count + 1, sizeof(s_dw_attr))) return 0;
			ab->attrs[ab->attr_count++] = at;
			a.attr_count++;
		}
		if (!s_dw_grow((void**)&ab->items, &ab->cap, ab->count + 1, sizeof(s_dw_abbrev))) return 0;
		if (code != (uint64_t)ab->count + 1) ab->dense = 0;
		ab->items[ab->count++] = a;
	}
	return 1;
}

static const s_dw_abbrev* s_dw_abbrev_find(const s_dw_abbrevs* ab, uint64_t code, int* index)
{
	if (ab->dense) {
		if (code >= 1 && code <= (uint64_t)ab->count) { *index = (int)code - 1; return &ab->items[code - 1]; }
		return NULL;
	}
	for (int i = 0; i < ab->count; ++i) if (ab->items[i].code == code) { *index = i; return &ab->items[i]; }
	return NULL;
}

/* An attribute value with its class resolved as far as the form allows. Strings and addresses that
   live in index tables (strx, addrx) are resolved later through the unit's bases. */
enum
{
	S_DW_CLS_NONE, S_DW_CLS_ADDR, S_DW_CLS_ADDRX, S_DW_CLS_CONST, S_DW_CLS_SCONST, S_DW_CLS_STR, S_DW_CLS_STRX,
	S_DW_CLS_REF, S_DW_CLS_SEC_OFFSET, S_DW_CLS_BLOCK, S_DW_CLS_FLAG, S_DW_CLS_RNGLISTX, S_DW_CLS_OTHER
};

typedef struct s_dw_val { int cls; uint64_t u; int64_t s; const char* str; } s_dw_val;

static void s_dw_read_form(s_dw_ctx* ctx, const s_dw_cu* cu, s_dw_cur* c, uint16_t form, int64_t implicit_const, s_dw_val* v)
{
	v->cls = S_DW_CLS_NONE; v->u = 0; v->s = 0; v->str = NULL;
	for (int indirect = 0; indirect < 4; ++indirect) {
		switch (form) {
		case S_DW_FORM_indirect: form = (uint16_t)s_dw_uleb(c); continue;
		case S_DW_FORM_addr: v->cls = S_DW_CLS_ADDR; v->u = s_dw_sized(c, cu->addr_size); return;
		case S_DW_FORM_addrx: v->cls = S_DW_CLS_ADDRX; v->u = s_dw_uleb(c); return;
		case S_DW_FORM_addrx1: v->cls = S_DW_CLS_ADDRX; v->u = s_dw_u8(c); return;
		case S_DW_FORM_addrx2: v->cls = S_DW_CLS_ADDRX; v->u = s_dw_u16(c); return;
		case S_DW_FORM_addrx3: v->cls = S_DW_CLS_ADDRX; v->u = s_dw_u16(c); v->u |= (uint64_t)s_dw_u8(c) << 16; return;
		case S_DW_FORM_addrx4: v->cls = S_DW_CLS_ADDRX; v->u = s_dw_u32(c); return;
		case S_DW_FORM_GNU_addr_index: v->cls = S_DW_CLS_ADDRX; v->u = s_dw_uleb(c); return;
		case S_DW_FORM_data1: v->cls = S_DW_CLS_CONST; v->u = s_dw_u8(c); return;
		case S_DW_FORM_data2: v->cls = S_DW_CLS_CONST; v->u = s_dw_u16(c); return;
		case S_DW_FORM_data4: v->cls = S_DW_CLS_CONST; v->u = s_dw_u32(c); return;
		case S_DW_FORM_data8: v->cls = S_DW_CLS_CONST; v->u = s_dw_u64(c); return;
		case S_DW_FORM_data16: v->cls = S_DW_CLS_OTHER; s_dw_skip(c, 16); return;
		case S_DW_FORM_udata: v->cls = S_DW_CLS_CONST; v->u = s_dw_uleb(c); return;
		case S_DW_FORM_sdata: v->cls = S_DW_CLS_SCONST; v->s = s_dw_sleb(c); v->u = (uint64_t)v->s; return;
		case S_DW_FORM_implicit_const: v->cls = S_DW_CLS_SCONST; v->s = implicit_const; v->u = (uint64_t)implicit_const; return;
		case S_DW_FORM_flag: v->cls = S_DW_CLS_FLAG; v->u = s_dw_u8(c); return;
		case S_DW_FORM_flag_present: v->cls = S_DW_CLS_FLAG; v->u = 1; return;
		case S_DW_FORM_string: v->cls = S_DW_CLS_STR; v->str = s_dw_cstr(c); return;
		case S_DW_FORM_strp: v->cls = S_DW_CLS_STR; v->str = s_dw_str_at(&ctx->secs->str, s_dw_sized(c, cu->offset_size)); return;
		case S_DW_FORM_line_strp: v->cls = S_DW_CLS_STR; v->str = s_dw_str_at(&ctx->secs->line_str, s_dw_sized(c, cu->offset_size)); return;
		case S_DW_FORM_strp_sup: case S_DW_FORM_GNU_strp_alt: v->cls = S_DW_CLS_OTHER; s_dw_sized(c, cu->offset_size); return;
		case S_DW_FORM_strx: case S_DW_FORM_GNU_str_index: v->cls = S_DW_CLS_STRX; v->u = s_dw_uleb(c); return;
		case S_DW_FORM_strx1: v->cls = S_DW_CLS_STRX; v->u = s_dw_u8(c); return;
		case S_DW_FORM_strx2: v->cls = S_DW_CLS_STRX; v->u = s_dw_u16(c); return;
		case S_DW_FORM_strx3: v->cls = S_DW_CLS_STRX; v->u = s_dw_u16(c); v->u |= (uint64_t)s_dw_u8(c) << 16; return;
		case S_DW_FORM_strx4: v->cls = S_DW_CLS_STRX; v->u = s_dw_u32(c); return;
		case S_DW_FORM_ref1: v->cls = S_DW_CLS_REF; v->u = cu->offset + s_dw_u8(c); return;
		case S_DW_FORM_ref2: v->cls = S_DW_CLS_REF; v->u = cu->offset + s_dw_u16(c); return;
		case S_DW_FORM_ref4: v->cls = S_DW_CLS_REF; v->u = cu->offset + s_dw_u32(c); return;
		case S_DW_FORM_ref8: v->cls = S_DW_CLS_REF; v->u = cu->offset + s_dw_u64(c); return;
		case S_DW_FORM_ref_udata: v->cls = S_DW_CLS_REF; v->u = cu->offset + s_dw_uleb(c); return;
		case S_DW_FORM_ref_addr: v->cls = S_DW_CLS_REF; v->u = cu->version <= 2 ? s_dw_sized(c, cu->addr_size) : s_dw_sized(c, cu->offset_size); return;
		case S_DW_FORM_ref_sup4: v->cls = S_DW_CLS_OTHER; s_dw_u32(c); return;
		case S_DW_FORM_ref_sup8: v->cls = S_DW_CLS_OTHER; s_dw_u64(c); return;
		case S_DW_FORM_GNU_ref_alt: v->cls = S_DW_CLS_OTHER; s_dw_sized(c, cu->offset_size); return;
		case S_DW_FORM_ref_sig8: v->cls = S_DW_CLS_OTHER; s_dw_u64(c); return;
		case S_DW_FORM_sec_offset: v->cls = S_DW_CLS_SEC_OFFSET; v->u = s_dw_sized(c, cu->offset_size); return;
		case S_DW_FORM_loclistx: v->cls = S_DW_CLS_OTHER; s_dw_uleb(c); return;
		case S_DW_FORM_rnglistx: v->cls = S_DW_CLS_RNGLISTX; v->u = s_dw_uleb(c); return;
		case S_DW_FORM_exprloc: case S_DW_FORM_block: v->cls = S_DW_CLS_BLOCK; s_dw_skip(c, s_dw_uleb(c)); return;
		case S_DW_FORM_block1: v->cls = S_DW_CLS_BLOCK; s_dw_skip(c, s_dw_u8(c)); return;
		case S_DW_FORM_block2: v->cls = S_DW_CLS_BLOCK; s_dw_skip(c, s_dw_u16(c)); return;
		case S_DW_FORM_block4: v->cls = S_DW_CLS_BLOCK; s_dw_skip(c, s_dw_u32(c)); return;
		default: c->ok = 0; return;
		}
	}
	c->ok = 0;
}

static const char* s_dw_val_str(s_dw_ctx* ctx, const s_dw_cu* cu, const s_dw_val* v)
{
	if (v->cls == S_DW_CLS_STR) return v->str;
	if (v->cls != S_DW_CLS_STRX) return NULL;
	const s_dw_sec* so = &ctx->secs->str_offsets;
	uint64_t base = cu->has_str_offsets_base ? cu->str_offsets_base : (cu->version >= 5 ? 8 : 0);
	uint64_t at = base + v->u * cu->offset_size;
	if (!so->data || at + cu->offset_size > so->len) return NULL;
	s_dw_cur c = s_dw_cur_make(so->data + at, cu->offset_size);
	return s_dw_str_at(&ctx->secs->str, s_dw_sized(&c, cu->offset_size));
}

static int s_dw_val_addr(s_dw_ctx* ctx, const s_dw_cu* cu, const s_dw_val* v, uint64_t* out)
{
	if (v->cls == S_DW_CLS_ADDR) { *out = v->u; return 1; }
	if (v->cls != S_DW_CLS_ADDRX) return 0;
	const s_dw_sec* as = &ctx->secs->addr;
	uint64_t base = cu->has_addr_base ? cu->addr_base : 8;
	uint64_t at = base + v->u * cu->addr_size;
	if (!as->data || at + cu->addr_size > as->len) return 0;
	s_dw_cur c = s_dw_cur_make(as->data + at, cu->addr_size);
	*out = s_dw_sized(&c, cu->addr_size);
	return c.ok;
}

/* Reads the wanted attributes of the DIE at `off` into vals (NONE where absent); cursor left after the DIE. */
static int s_dw_die_attrs(s_dw_ctx* ctx, const s_dw_cu* cu, uint64_t off, const uint16_t* wanted, int wanted_count, s_dw_val* vals, const s_dw_abbrev** abbrev_out)
{
	const s_dw_sec* info = cu->sec;
	for (int i = 0; i < wanted_count; ++i) { vals[i].cls = S_DW_CLS_NONE; vals[i].u = 0; vals[i].s = 0; vals[i].str = NULL; }
	if (off < cu->die_offset || off >= cu->end) return 0;
	s_dw_cur c = s_dw_cur_make(info->data + off, (size_t)(cu->end - off));
	uint64_t code = s_dw_uleb(&c);
	int index = 0;
	const s_dw_abbrev* ab = code ? s_dw_abbrev_find(&cu->abbrevs, code, &index) : NULL;
	if (!ab) return 0;
	if (abbrev_out) *abbrev_out = ab;
	for (uint32_t i = 0; i < ab->attr_count; ++i) {
		const s_dw_attr* at = &cu->abbrevs.attrs[ab->attr_start + i];
		s_dw_val v;
		s_dw_read_form(ctx, cu, &c, at->form, at->implicit_const, &v);
		if (!c.ok) return 0;
		for (int w = 0; w < wanted_count; ++w) if (wanted[w] == at->name) { vals[w] = v; break; }
	}
	return 1;
}

static void s_dw_cu_unload(s_dw_cu* cu)
{
	free(cu->abbrevs.items); free(cu->abbrevs.attrs);
	memset(&cu->abbrevs, 0, sizeof(cu->abbrevs));
	free(cu->dies); cu->dies = NULL; cu->die_count = cu->die_cap = 0;
	for (int i = 0; i < cu->file_count; ++i) free(cu->files[i]);
	free(cu->files); cu->files = NULL; cu->file_count = cu->file_cap = 0;
	cu->files_done = 0;
	cu->loaded = 0;
}

/* Walks the unit's DIEs into a flat array with parent links and reads the unit DIE's attributes. */
static int s_dw_cu_load(s_dw_ctx* ctx, s_dw_cu* cu)
{
	if (cu->loaded) { cu->use = ++ctx->use_seq; return 1; }
	/* Keep the cache small: unload the least recently used of the others when more than four are up. */
	int loaded = 0;
	for (int i = 0; i < ctx->cu_count; ++i) if (ctx->cus[i].loaded) loaded++;
	while (loaded > 4) {
		int victim = -1;
		for (int i = 0; i < ctx->cu_count; ++i) {
			if (!ctx->cus[i].loaded || i == ctx->current) continue;
			if (victim < 0 || ctx->cus[i].use < ctx->cus[victim].use) victim = i;
		}
		if (victim < 0) break;
		s_dw_cu_unload(&ctx->cus[victim]);
		loaded--;
	}
	if (!s_dw_abbrevs_load(ctx, cu)) { s_dw_cu_unload(cu); return 0; }
	const s_dw_sec* info = cu->sec;
	s_dw_cur c = s_dw_cur_make(info->data + cu->die_offset, (size_t)(cu->end - cu->die_offset));
	int32_t stack[S_DW_MAX_DEPTH];
	int depth = 0;
	int32_t parent = -1;
	while (c.ok && (uint64_t)(c.p - info->data) < cu->end) {
		uint64_t off = (uint64_t)(c.p - info->data);
		uint64_t code = s_dw_uleb(&c);
		if (!c.ok) break;
		if (code == 0) {
			if (depth == 0) break; /* the unit ends with its own null entry */
			parent = stack[--depth];
			continue;
		}
		int index = 0;
		const s_dw_abbrev* ab = s_dw_abbrev_find(&cu->abbrevs, code, &index);
		if (!ab) { s_dw_warn(ctx, "dwarf: unknown abbreviation %s%llu", "code ", code); break; }
		s_dw_die d;
		d.offset = off;
		d.parent = parent;
		d.abbrev = index;
		d.tag = ab->tag;
		d.has_children = ab->has_children;
		d.depth = (uint8_t)depth;
		if (!s_dw_grow((void**)&cu->dies, &cu->die_cap, cu->die_count + 1, sizeof(s_dw_die))) { s_dw_cu_unload(cu); return 0; }
		int32_t me = cu->die_count;
		cu->dies[cu->die_count++] = d;
		for (uint32_t i = 0; i < ab->attr_count; ++i) {
			const s_dw_attr* at = &cu->abbrevs.attrs[ab->attr_start + i];
			s_dw_val v;
			s_dw_read_form(ctx, cu, &c, at->form, at->implicit_const, &v);
			if (!c.ok) { s_dw_warn(ctx, "dwarf: unreadable attribute form in DIE at %s0x%llx; the unit's remaining DIEs are skipped", "", off); break; }
		}
		if (!c.ok) break;
		if (ab->has_children) {
			if (depth >= S_DW_MAX_DEPTH) { s_dw_warn(ctx, "dwarf: DIE nesting past %s%llu, unit skipped", "", (uint64_t)S_DW_MAX_DEPTH); s_dw_cu_unload(cu); return 0; }
			stack[depth++] = parent;
			parent = me;
		}
	}
	if (cu->die_count == 0) { s_dw_cu_unload(cu); return 0; }
	/* The unit DIE: bases first, since its own strings may be strx. */
	static const uint16_t wanted[] = { S_DW_AT_str_offsets_base, S_DW_AT_addr_base, S_DW_AT_rnglists_base, S_DW_AT_low_pc, S_DW_AT_stmt_list, S_DW_AT_name, S_DW_AT_comp_dir, S_DW_AT_entry_pc };
	s_dw_val vals[8];
	if (s_dw_die_attrs(ctx, cu, cu->dies[0].offset, wanted, 8, vals, NULL)) {
		if (vals[0].cls == S_DW_CLS_SEC_OFFSET) { cu->str_offsets_base = vals[0].u; cu->has_str_offsets_base = 1; }
		if (vals[1].cls == S_DW_CLS_SEC_OFFSET) { cu->addr_base = vals[1].u; cu->has_addr_base = 1; }
		if (vals[2].cls == S_DW_CLS_SEC_OFFSET) { cu->rnglists_base = vals[2].u; cu->has_rnglists_base = 1; }
		if (s_dw_val_addr(ctx, cu, &vals[3], &cu->low_pc)) cu->has_low_pc = 1;
		else if (s_dw_val_addr(ctx, cu, &vals[7], &cu->low_pc)) cu->has_low_pc = 1;
		if (vals[4].cls == S_DW_CLS_SEC_OFFSET || vals[4].cls == S_DW_CLS_CONST) { cu->stmt_list = vals[4].u; cu->has_stmt_list = 1; }
		cu->name = s_dw_val_str(ctx, cu, &vals[5]);
		cu->comp_dir = s_dw_val_str(ctx, cu, &vals[6]);
	}
	cu->loaded = 1;
	cu->use = ++ctx->use_seq;
	return 1;
}

static int s_dw_cu_find(s_dw_ctx* ctx, uint64_t off)
{
	int lo = 0, hi = ctx->cu_count - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		if (off < ctx->cus[mid].offset) hi = mid - 1;
		else if (off >= ctx->cus[mid].end) lo = mid + 1;
		else return mid;
	}
	return -1;
}

static int s_dw_die_find(const s_dw_cu* cu, uint64_t off)
{
	int lo = 0, hi = cu->die_count - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		if (off < cu->dies[mid].offset) hi = mid - 1;
		else if (off > cu->dies[mid].offset) lo = mid + 1;
		else return mid;
	}
	return -1;
}

/* ---------------------------------------------------------------------------------------------- */
/* Ranges. */

typedef struct s_dw_range { uint64_t lo, hi; } s_dw_range;
typedef struct s_dw_ranges { s_dw_range* items; int count, cap; } s_dw_ranges;

static int s_dw_ranges_add(s_dw_ranges* r, uint64_t lo, uint64_t hi)
{
	if (hi <= lo) return 1;
	if (!s_dw_grow((void**)&r->items, &r->cap, r->count + 1, sizeof(s_dw_range))) return 0;
	r->items[r->count].lo = lo;
	r->items[r->count].hi = hi;
	r->count++;
	return 1;
}

static int s_dw_read_rnglist(s_dw_ctx* ctx, const s_dw_cu* cu, uint64_t off, s_dw_ranges* out)
{
	const s_dw_sec* sec = &ctx->secs->rnglists;
	if (!sec->data || off >= sec->len) return 0;
	s_dw_cur c = s_dw_cur_make(sec->data + off, sec->len - (size_t)off);
	uint64_t base = cu->has_low_pc ? cu->low_pc : 0;
	for (int n = 0; n < 100000 && c.ok; ++n) {
		uint8_t kind = s_dw_u8(&c);
		if (!c.ok) return 0;
		s_dw_val v;
		uint64_t a, b;
		switch (kind) {
		case 0: return 1;
		case 1: v.cls = S_DW_CLS_ADDRX; v.u = s_dw_uleb(&c); if (s_dw_val_addr(ctx, cu, &v, &a)) base = a; break;
		case 2: {
			s_dw_val w;
			v.cls = S_DW_CLS_ADDRX; v.u = s_dw_uleb(&c);
			w.cls = S_DW_CLS_ADDRX; w.u = s_dw_uleb(&c);
			if (s_dw_val_addr(ctx, cu, &v, &a) && s_dw_val_addr(ctx, cu, &w, &b) && !s_dw_ranges_add(out, a, b)) return 0;
			break;
		}
		case 3: v.cls = S_DW_CLS_ADDRX; v.u = s_dw_uleb(&c); b = s_dw_uleb(&c); if (s_dw_val_addr(ctx, cu, &v, &a) && !s_dw_ranges_add(out, a, a + b)) return 0; break;
		case 4: a = s_dw_uleb(&c); b = s_dw_uleb(&c); if (!s_dw_ranges_add(out, base + a, base + b)) return 0; break;
		case 5: base = s_dw_sized(&c, cu->addr_size); break;
		case 6: a = s_dw_sized(&c, cu->addr_size); b = s_dw_sized(&c, cu->addr_size); if (!s_dw_ranges_add(out, a, b)) return 0; break;
		case 7: a = s_dw_sized(&c, cu->addr_size); b = s_dw_uleb(&c); if (!s_dw_ranges_add(out, a, a + b)) return 0; break;
		default: return 0;
		}
	}
	return 0;
}

static int s_dw_read_ranges_v4(s_dw_ctx* ctx, const s_dw_cu* cu, uint64_t off, s_dw_ranges* out)
{
	const s_dw_sec* sec = &ctx->secs->ranges;
	if (!sec->data || off >= sec->len) return 0;
	s_dw_cur c = s_dw_cur_make(sec->data + off, sec->len - (size_t)off);
	uint64_t base = cu->has_low_pc ? cu->low_pc : 0;
	uint64_t max = cu->addr_size == 8 ? ~(uint64_t)0 : 0xffffffffu;
	for (int n = 0; n < 100000 && c.ok; ++n) {
		uint64_t a = s_dw_sized(&c, cu->addr_size);
		uint64_t b = s_dw_sized(&c, cu->addr_size);
		if (!c.ok) return 0;
		if (a == 0 && b == 0) return 1;
		if (a == max) { base = b; continue; }
		if (!s_dw_ranges_add(out, base + a, base + b)) return 0;
	}
	return 0;
}

/* The code ranges of a DIE from low_pc/high_pc or DW_AT_ranges, in DWARF addresses (not rebased). */
static int s_dw_die_ranges(s_dw_ctx* ctx, const s_dw_cu* cu, const s_dw_val* low, const s_dw_val* high, const s_dw_val* ranges, s_dw_ranges* out)
{
	out->count = 0;
	uint64_t lo;
	if (s_dw_val_addr(ctx, cu, low, &lo)) {
		uint64_t hi;
		if (s_dw_val_addr(ctx, cu, high, &hi)) return s_dw_ranges_add(out, lo, hi);
		if (high->cls == S_DW_CLS_CONST || high->cls == S_DW_CLS_SCONST) return s_dw_ranges_add(out, lo, lo + high->u);
		return 1;
	}
	if (ranges->cls == S_DW_CLS_RNGLISTX) {
		const s_dw_sec* sec = &ctx->secs->rnglists;
		uint64_t base = cu->has_rnglists_base ? cu->rnglists_base : (cu->dwarf64 ? 20 : 12);
		uint64_t at = base + ranges->u * cu->offset_size;
		if (!sec->data || at + cu->offset_size > sec->len) return 1;
		s_dw_cur c = s_dw_cur_make(sec->data + at, cu->offset_size);
		uint64_t rel = s_dw_sized(&c, cu->offset_size);
		s_dw_read_rnglist(ctx, cu, base + rel, out); /* a damaged list leaves what was read; the DIE is still emitted */
		return 1;
	}
	if (ranges->cls == S_DW_CLS_SEC_OFFSET || ranges->cls == S_DW_CLS_CONST) {
		if (cu->version >= 5) s_dw_read_rnglist(ctx, cu, ranges->u, out);
		else s_dw_read_ranges_v4(ctx, cu, ranges->u, out);
		return 1;
	}
	return 1;
}

/* ---------------------------------------------------------------------------------------------- */
/* Names. The name of a subprogram or inlined call may sit on the DIE itself, on its declaration
   (DW_AT_specification) or on its abstract instance (DW_AT_abstract_origin), possibly in another
   unit. The scope comes from the parents of whichever DIE carries the name. */

#define S_DW_NAME_CAP 1024

typedef struct s_dw_named { s_dw_cu* cu; int die; const char* name; int linkage; } s_dw_named;

static int s_dw_resolve_ref(s_dw_ctx* ctx, uint64_t off, s_dw_cu** cu_out, int* die_out)
{
	int ci = s_dw_cu_find(ctx, off);
	if (ci < 0) return 0;
	s_dw_cu* cu = &ctx->cus[ci];
	if (!s_dw_cu_load(ctx, cu)) return 0;
	int di = s_dw_die_find(cu, off);
	if (di < 0) return 0;
	*cu_out = cu;
	*die_out = di;
	return 1;
}

static int s_dw_named_die(s_dw_ctx* ctx, s_dw_cu* cu, int die, s_dw_named* out, int hops)
{
	static const uint16_t wanted[] = { S_DW_AT_name, S_DW_AT_specification, S_DW_AT_abstract_origin, S_DW_AT_linkage_name, S_DW_AT_MIPS_linkage_name };
	s_dw_val v[5];
	if (hops > 8) return 0;
	if (!s_dw_die_attrs(ctx, cu, cu->dies[die].offset, wanted, 5, v, NULL)) return 0;
	const char* name = s_dw_val_str(ctx, cu, &v[0]);
	if (name && name[0]) { out->cu = cu; out->die = die; out->name = name; out->linkage = 0; return 1; }
	if (v[1].cls == S_DW_CLS_REF || v[2].cls == S_DW_CLS_REF) {
		s_dw_cu* tcu; int tdie;
		uint64_t ref = v[1].cls == S_DW_CLS_REF ? v[1].u : v[2].u;
		if (s_dw_resolve_ref(ctx, ref, &tcu, &tdie) && s_dw_named_die(ctx, tcu, tdie, out, hops + 1)) return 1;
		if (v[1].cls == S_DW_CLS_REF && v[2].cls == S_DW_CLS_REF && s_dw_resolve_ref(ctx, v[2].u, &tcu, &tdie) && s_dw_named_die(ctx, tcu, tdie, out, hops + 1)) return 1;
	}
	const char* linkage = s_dw_val_str(ctx, cu, &v[3]);
	if (!linkage) linkage = s_dw_val_str(ctx, cu, &v[4]);
	if (linkage && linkage[0]) { out->cu = cu; out->die = die; out->name = linkage; out->linkage = 1; return 1; }
	return 0;
}

static void s_dw_append(char* buf, size_t cap, size_t* len, const char* s)
{
	size_t n = strlen(s);
	if (*len + n >= cap) n = cap - *len - 1;
	memcpy(buf + *len, s, n);
	*len += n;
	buf[*len] = 0;
}

#define S_DW_AT_signature 0x69

/* The name of the type a signature stands for: a type unit's type DIE. With -fdebug-types-section
   a class in the compile unit is only a skeleton carrying the signature, and its name lives here. */
static const char* s_dw_type_unit_name(s_dw_ctx* ctx, uint64_t signature)
{
	static const uint16_t wanted[] = { S_DW_AT_name };
	for (int i = 0; i < ctx->tu_count; ++i) {
		s_dw_cu* tu = &ctx->tus[i];
		if (tu->signature != signature) continue;
		if (!s_dw_cu_load(ctx, tu)) return NULL;
		s_dw_val v;
		if (!s_dw_die_attrs(ctx, tu, tu->offset + tu->type_offset, wanted, 1, &v, NULL)) return NULL;
		return s_dw_val_str(ctx, tu, &v);
	}
	return NULL;
}

static int s_dw_named_die(s_dw_ctx* ctx, s_dw_cu* cu, int die, s_dw_named* out, int hops);

/* The scope-qualified name into buf: namespaces and types enclosing the named DIE, then the name.
   A local class or lambda is scoped by the function holding it, whose own qualified name ends the
   walk upward. */
static void s_dw_qualified_name_depth(s_dw_ctx* ctx, const s_dw_named* nm, char* buf, size_t cap, int depth)
{
	buf[0] = 0;
	size_t len = 0;
	if (nm->linkage) { s_dw_append(buf, cap, &len, nm->name); return; }
	static const uint16_t wanted[] = { S_DW_AT_name, S_DW_AT_signature };
	const char* parts[64];
	char outer[S_DW_NAME_CAP];
	int count = 0;
	int32_t p = nm->cu->dies[nm->die].parent;
	while (p >= 0 && count < 64) {
		const s_dw_die* d = &nm->cu->dies[p];
		uint16_t t = d->tag;
		if (t == S_DW_TAG_subprogram && depth < 4) {
			s_dw_named outer_nm;
			if (s_dw_named_die(ctx, nm->cu, p, &outer_nm, 0)) {
				s_dw_qualified_name_depth(ctx, &outer_nm, outer, sizeof(outer), depth + 1);
				if (outer[0]) { parts[count++] = outer; break; }
			}
		}
		if (t == S_DW_TAG_namespace || t == S_DW_TAG_class_type || t == S_DW_TAG_structure_type || t == S_DW_TAG_union_type || t == S_DW_TAG_enumeration_type) {
			s_dw_val v[2];
			const char* n = NULL;
			if (s_dw_die_attrs(ctx, nm->cu, d->offset, wanted, 2, v, NULL)) {
				n = s_dw_val_str(ctx, nm->cu, &v[0]);
				if ((!n || !n[0]) && v[1].cls == S_DW_CLS_OTHER) {
					/* ref_sig8 reads as OTHER; the signature itself is re-read from the DIE's bytes. */
					s_dw_cur sc = s_dw_cur_make(nm->cu->sec->data + d->offset, (size_t)(nm->cu->end - d->offset));
					uint64_t code = s_dw_uleb(&sc);
					int ai = 0;
					const s_dw_abbrev* ab = s_dw_abbrev_find(&nm->cu->abbrevs, code, &ai);
					for (uint32_t i = 0; ab && i < ab->attr_count; ++i) {
						const s_dw_attr* at = &nm->cu->abbrevs.attrs[ab->attr_start + i];
						if (at->name == S_DW_AT_signature && at->form == S_DW_FORM_ref_sig8) { n = s_dw_type_unit_name(ctx, s_dw_u64(&sc)); break; }
						s_dw_val skip;
						s_dw_read_form(ctx, nm->cu, &sc, at->form, at->implicit_const, &skip);
						if (!sc.ok) break;
					}
				}
			}
			if (!n || !n[0]) n = t == S_DW_TAG_namespace ? "(anonymous namespace)" : "(anonymous)";
			parts[count++] = n;
		}
		p = d->parent;
	}
	for (int i = count - 1; i >= 0; --i) { s_dw_append(buf, cap, &len, parts[i]); s_dw_append(buf, cap, &len, "::"); }
	s_dw_append(buf, cap, &len, nm->name);
}

static void s_dw_qualified_name(s_dw_ctx* ctx, const s_dw_named* nm, char* buf, size_t cap)
{
	s_dw_qualified_name_depth(ctx, nm, buf, cap, 0);
}

/* ---------------------------------------------------------------------------------------------- */
/* Line programs. */

typedef struct s_dw_line_hdr
{
	uint16_t version;
	uint8_t offset_size, addr_size, min_inst, max_ops, default_is_stmt, line_range, opcode_base;
	int8_t line_base;
	const uint8_t* std_lengths; /* opcode_base - 1 entries */
	const unsigned char* program; size_t program_len;
	int dwarf64;
} s_dw_line_hdr;

static int s_dw_cu_file_add(s_dw_cu* cu, const char* path)
{
	char* d = s_dw_strdup(path);
	if (!d) return 0;
	if (!s_dw_grow((void**)&cu->files, &cu->file_cap, cu->file_count + 1, sizeof(char*))) { free(d); return 0; }
	cu->files[cu->file_count++] = d;
	return 1;
}

/* One v5 entry-format list: reads an entry's path and directory index, skipping the rest. */
static int s_dw_read_v5_entry(s_dw_ctx* ctx, const s_dw_cu* cu, s_dw_cur* c, const uint16_t* formats, int format_count, const char** path, uint64_t* dir_index)
{
	*path = NULL;
	*dir_index = 0;
	for (int i = 0; i < format_count; ++i) {
		uint16_t content = formats[i * 2], form = formats[i * 2 + 1];
		s_dw_val v;
		s_dw_read_form(ctx, cu, c, form, 0, &v);
		if (!c->ok) return 0;
		if (content == 1) *path = s_dw_val_str(ctx, cu, &v);
		else if (content == 2) *dir_index = v.u;
	}
	return 1;
}

/* Parses the header at the unit's stmt_list, filling the unit's file list; false when absent or damaged. */
static int s_dw_line_header(s_dw_ctx* ctx, s_dw_cu* cu, s_dw_line_hdr* h)
{
	const s_dw_sec* sec = &ctx->secs->line;
	if (!cu->has_stmt_list || !sec->data || cu->stmt_list >= sec->len) return 0;
	s_dw_cur c = s_dw_cur_make(sec->data + cu->stmt_list, sec->len - (size_t)cu->stmt_list);
	memset(h, 0, sizeof(*h));
	uint64_t length = s_dw_u32(&c);
	if (length == 0xffffffffu) { length = s_dw_u64(&c); h->dwarf64 = 1; h->offset_size = 8; }
	else h->offset_size = 4;
	if (!c.ok || length > (uint64_t)(c.end - c.p)) return 0;
	const unsigned char* unit_end = c.p + length;
	h->version = s_dw_u16(&c);
	if (h->version < 2 || h->version > 5) return 0;
	if (h->version >= 5) { h->addr_size = s_dw_u8(&c); s_dw_u8(&c); /* segment selector size */ }
	else h->addr_size = cu->addr_size;
	uint64_t header_length = s_dw_sized(&c, h->offset_size);
	if (!c.ok || header_length > (uint64_t)(unit_end - c.p)) return 0;
	const unsigned char* program = c.p + header_length;
	h->min_inst = s_dw_u8(&c);
	h->max_ops = h->version >= 4 ? s_dw_u8(&c) : 1;
	h->default_is_stmt = s_dw_u8(&c);
	h->line_base = (int8_t)s_dw_u8(&c);
	h->line_range = s_dw_u8(&c);
	h->opcode_base = s_dw_u8(&c);
	if (!c.ok || h->opcode_base == 0 || h->line_range == 0 || !s_dw_left(&c, (size_t)h->opcode_base - 1)) return 0;
	h->std_lengths = c.p;
	s_dw_skip(&c, (uint64_t)h->opcode_base - 1);
	/* A line unit of its own: the file list belongs to the unit that owns the DWARF, so a shared
	   stmt_list (one unit) is parsed once; the cursor rules are per unit. */
	s_dw_cu lcu = *cu; /* a copy with the line header's offset size for strp/line_strp forms */
	lcu.offset_size = h->offset_size;
	lcu.addr_size = h->addr_size;
	if (h->version >= 5) {
		uint16_t dformats[32], fformats[32];
		uint8_t dfc = s_dw_u8(&c);
		if (dfc > 16) return 0;
		for (int i = 0; i < dfc; ++i) { dformats[i * 2] = (uint16_t)s_dw_uleb(&c); dformats[i * 2 + 1] = (uint16_t)s_dw_uleb(&c); }
		uint64_t dcount = s_dw_uleb(&c);
		if (!c.ok || dcount > 100000) return 0;
		char** dirs = (char**)calloc((size_t)dcount + 1, sizeof(char*));
		if (!dirs) return 0;
		int ok = 1;
		for (uint64_t i = 0; i < dcount && ok; ++i) {
			const char* path; uint64_t di;
			ok = s_dw_read_v5_entry(ctx, &lcu, &c, dformats, dfc, &path, &di);
			if (ok) dirs[i] = s_dw_strdup(path ? path : "");
		}
		uint8_t ffc = ok ? s_dw_u8(&c) : 0;
		if (ffc > 16) ok = 0;
		for (int i = 0; i < ffc && ok; ++i) { fformats[i * 2] = (uint16_t)s_dw_uleb(&c); fformats[i * 2 + 1] = (uint16_t)s_dw_uleb(&c); }
		uint64_t fcount = ok ? s_dw_uleb(&c) : 0;
		if (!c.ok || fcount > 1000000) ok = 0;
		for (uint64_t i = 0; i < fcount && ok; ++i) {
			const char* path; uint64_t di;
			ok = s_dw_read_v5_entry(ctx, &lcu, &c, fformats, ffc, &path, &di);
			if (ok) {
				char full[2048];
				const char* dir = di < dcount ? dirs[di] : NULL;
				/* Directory 0 is the compilation directory itself; a relative entry under it joins comp_dir. */
				s_dw_path_join(full, sizeof(full), di == 0 ? NULL : cu->comp_dir, dir, path ? path : "");
				if (di == 0 && dir && (full[0] != '/' && full[0] != '\\' && !(full[0] && full[1] == ':'))) {
					char again[2048];
					s_dw_path_join(again, sizeof(again), NULL, dir, path ? path : "");
					memcpy(full, again, sizeof(full));
				}
				ok = s_dw_cu_file_add(cu, full);
			}
		}
		for (uint64_t i = 0; i < dcount; ++i) free(dirs[i]);
		free(dirs);
		if (!ok || !c.ok) return 0;
	} else {
		char** dirs = NULL; int dcount = 0, dcap = 0;
		if (!s_dw_grow((void**)&dirs, &dcap, 1, sizeof(char*))) return 0;
		dirs[dcount++] = NULL; /* index 0: the compilation directory */
		int ok = 1;
		for (;;) {
			const char* d = s_dw_cstr(&c);
			if (!d) { ok = 0; break; }
			if (!d[0]) break;
			if (!s_dw_grow((void**)&dirs, &dcap, dcount + 1, sizeof(char*))) { ok = 0; break; }
			dirs[dcount++] = s_dw_strdup(d);
		}
		/* Index 0 names the unit's primary file; v2-4 file numbers start at 1. */
		if (ok) ok = s_dw_cu_file_add(cu, cu->name ? cu->name : "");
		while (ok) {
			const char* name = s_dw_cstr(&c);
			if (!name) { ok = 0; break; }
			if (!name[0]) break;
			uint64_t di = s_dw_uleb(&c);
			s_dw_uleb(&c); s_dw_uleb(&c);
			if (!c.ok) { ok = 0; break; }
			char full[2048];
			s_dw_path_join(full, sizeof(full), cu->comp_dir, di < (uint64_t)dcount ? dirs[di] : NULL, name);
			ok = s_dw_cu_file_add(cu, full);
		}
		for (int i = 0; i < dcount; ++i) free(dirs[i]);
		free(dirs);
		if (!ok) return 0;
	}
	h->program = program;
	h->program_len = (size_t)(unit_end - program);
	cu->files_done = 1;
	return 1;
}

static const char* s_dw_cu_file(const s_dw_cu* cu, uint64_t index)
{
	if (index < (uint64_t)cu->file_count) return cu->files[index];
	return cu->file_count ? cu->files[0] : "";
}

static void s_dw_run_line_program(s_dw_ctx* ctx, s_dw_cu* cu, const s_dw_line_hdr* h)
{
	s_dw_cur c = s_dw_cur_make(h->program, h->program_len);
	uint64_t address = 0; uint64_t file = 1; int64_t line = 1; int is_stmt = h->default_is_stmt;
	int extra_files = 0;
	uint64_t open_span_end = ~(uint64_t)0;
	while (c.ok && c.p < c.end) {
		uint8_t op = s_dw_u8(&c);
		int emit = 0, end_seq = 0;
		if (op >= h->opcode_base) {
			uint8_t adj = (uint8_t)(op - h->opcode_base);
			address += (uint64_t)(adj / h->line_range) * h->min_inst;
			line += h->line_base + (int)(adj % h->line_range);
			emit = 1;
		} else if (op == 0) {
			uint64_t len = s_dw_uleb(&c);
			if (!c.ok || len == 0 || len > (uint64_t)(c.end - c.p)) break;
			const unsigned char* next = c.p + len;
			uint8_t sub = s_dw_u8(&c);
			switch (sub) {
			case 1: emit = 1; end_seq = 1; break;
			case 2: address = s_dw_sized(&c, len - 1 == 4 ? 4 : 8); break;
			case 3: {
				const char* name = s_dw_cstr(&c);
				uint64_t di = s_dw_uleb(&c);
				s_dw_uleb(&c); s_dw_uleb(&c);
				if (name && extra_files < 4096) {
					char full[2048];
					s_dw_path_join(full, sizeof(full), cu->comp_dir, NULL, name);
					(void)di;
					s_dw_cu_file_add(cu, full);
					extra_files++;
				}
				break;
			}
			default: break; /* set_discriminator and vendor extensions */
			}
			c.p = next;
		} else {
			switch (op) {
			case 1: emit = 1; break;
			case 2: address += s_dw_uleb(&c) * h->min_inst; break;
			case 3: line += s_dw_sleb(&c); break;
			case 4: file = s_dw_uleb(&c); break;
			case 5: s_dw_uleb(&c); break;
			case 6: is_stmt = !is_stmt; break;
			case 7: break;
			case 8: address += (uint64_t)((255 - h->opcode_base) / h->line_range) * h->min_inst; break;
			case 9: address += s_dw_u16(&c); break;
			case 10: case 11: break;
			case 12: s_dw_uleb(&c); break;
			default: {
				uint8_t n = h->std_lengths[op - 1];
				for (uint8_t i = 0; i < n; ++i) s_dw_uleb(&c);
				break;
			}
			}
		}
		if (!c.ok) break;
		if (emit) {
			uint64_t out, span_end;
			int mapped = s_dw_rebase_addr(ctx, address, &out, &span_end);
			/* Leaving the span of the previous rows: close them where that span ends. */
			if (open_span_end != ~(uint64_t)0 && (!mapped || span_end != open_span_end)) {
				sym_sink_line_end(ctx->sink, open_span_end);
				open_span_end = ~(uint64_t)0;
			}
			if (mapped) {
				if (end_seq) { sym_sink_line_end(ctx->sink, out); open_span_end = ~(uint64_t)0; }
				else if (is_stmt) { sym_sink_line(ctx->sink, out, 0, s_dw_cu_file(cu, file), line < 0 ? 0 : (uint32_t)line); open_span_end = span_end; }
			}
			if (end_seq) { address = 0; file = 1; line = 1; is_stmt = h->default_is_stmt; }
		}
	}
	if (open_span_end != ~(uint64_t)0) sym_sink_line_end(ctx->sink, open_span_end);
}

/* ---------------------------------------------------------------------------------------------- */
/* Functions and inlined calls. */

static void s_dw_emit_function(s_dw_ctx* ctx, const s_dw_ranges* r, const char* name)
{
	for (int i = 0; i < r->count; ++i) {
		uint64_t lo, hi;
		if (!s_dw_rebase_addr(ctx, r->items[i].lo, &lo, NULL)) continue;
		if (!s_dw_rebase_addr(ctx, r->items[i].hi, &hi, NULL)) hi = lo + (r->items[i].hi - r->items[i].lo);
		if (hi <= lo) continue;
		uint64_t size = hi - lo;
		sym_sink_function(ctx->sink, lo, size > 0xffffffffu ? 0xffffffffu : (uint32_t)size, name);
	}
}

static void s_dw_emit_inline(s_dw_ctx* ctx, const s_dw_ranges* r, const char* callee, const char* call_file, uint32_t call_line, int depth)
{
	for (int i = 0; i < r->count; ++i) {
		uint64_t lo, hi;
		if (!s_dw_rebase_addr(ctx, r->items[i].lo, &lo, NULL)) continue;
		if (!s_dw_rebase_addr(ctx, r->items[i].hi, &hi, NULL)) hi = lo + (r->items[i].hi - r->items[i].lo);
		if (hi <= lo) continue;
		uint64_t size = hi - lo;
		sym_sink_inline(ctx->sink, lo, size > 0xffffffffu ? 0xffffffffu : (uint32_t)size, callee, call_file, call_line, depth);
	}
}

static void s_dw_cu_emit(s_dw_ctx* ctx, s_dw_cu* cu)
{
	static const uint16_t wanted[] = { S_DW_AT_low_pc, S_DW_AT_high_pc, S_DW_AT_ranges, S_DW_AT_declaration, S_DW_AT_inline, S_DW_AT_call_file, S_DW_AT_call_line };
	s_dw_ranges r = { NULL, 0, 0 };
	char name[S_DW_NAME_CAP];
	for (int i = 1; i < cu->die_count; ++i) {
		const s_dw_die* d = &cu->dies[i];
		if (d->tag != S_DW_TAG_subprogram && d->tag != S_DW_TAG_inlined_subroutine) continue;
		s_dw_val v[7];
		if (!s_dw_die_attrs(ctx, cu, d->offset, wanted, 7, v, NULL)) continue;
		if (d->tag == S_DW_TAG_subprogram && v[3].cls == S_DW_CLS_FLAG && v[3].u) continue;
		if (!s_dw_die_ranges(ctx, cu, &v[0], &v[1], &v[2], &r) || r.count == 0) continue;
		s_dw_named nm;
		if (!s_dw_named_die(ctx, cu, i, &nm, 0)) { name[0] = 0; }
		else s_dw_qualified_name(ctx, &nm, name, sizeof(name));
		/* The cache may have unloaded a unit while resolving the name, but never the current one. */
		d = &cu->dies[i];
		if (d->tag == S_DW_TAG_subprogram) {
			s_dw_emit_function(ctx, &r, name);
		} else {
			int depth = 0;
			for (int32_t p = d->parent; p >= 0; p = cu->dies[p].parent) {
				if (cu->dies[p].tag == S_DW_TAG_inlined_subroutine) depth++;
				else if (cu->dies[p].tag == S_DW_TAG_subprogram) break;
			}
			const char* call_file = (v[5].cls == S_DW_CLS_CONST || v[5].cls == S_DW_CLS_SCONST) ? s_dw_cu_file(cu, v[5].u) : "";
			uint32_t call_line = (v[6].cls == S_DW_CLS_CONST || v[6].cls == S_DW_CLS_SCONST) ? (uint32_t)v[6].u : 0;
			s_dw_emit_inline(ctx, &r, name, call_file, call_line, depth);
		}
	}
	free(r.items);
}

/* Reads everything the sections hold into the sink. */
static int s_dw_read(const s_dw_sections* secs, const s_dw_rebase* rb, sym_sink* sink, sym_log_fn log, void* udata)
{
	s_dw_ctx ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.secs = secs; ctx.rb = rb; ctx.sink = sink; ctx.log = log; ctx.udata = udata; ctx.current = -1;
	if (!secs->info.data || !secs->abbrev.data) { sym_logf(log, udata, "dwarf: no .debug_info / .debug_abbrev"); return 0; }
	if (!s_dw_enumerate_units(&ctx)) { free(ctx.cus); return 0; }
	int units_ok = 0;
	for (int i = 0; i < ctx.cu_count; ++i) {
		s_dw_cu* cu = &ctx.cus[i];
		ctx.current = i;
		if (!s_dw_cu_load(&ctx, cu)) { s_dw_warn(&ctx, "dwarf: unit at %s0x%llx skipped", "", cu->offset); continue; }
		s_dw_line_hdr h;
		if (s_dw_line_header(&ctx, cu, &h)) s_dw_run_line_program(&ctx, cu, &h);
		else if (cu->has_stmt_list) s_dw_warn(&ctx, "dwarf: line table of unit at %s0x%llx unreadable", "", cu->offset);
		s_dw_cu_emit(&ctx, cu);
		units_ok++;
		s_dw_cu_unload(cu);
	}
	for (int i = 0; i < ctx.cu_count; ++i) s_dw_cu_unload(&ctx.cus[i]);
	for (int i = 0; i < ctx.tu_count; ++i) s_dw_cu_unload(&ctx.tus[i]);
	free(ctx.cus);
	free(ctx.tus);
	if (units_ok == 0) { sym_logf(log, udata, "dwarf: no readable units"); return 0; }
	return 1;
}

/* ---------------------------------------------------------------------------------------------- */
/* ELF. */

#define S_ELF_SHT_SYMTAB 2
#define S_ELF_SHT_NOTE 7
#define S_ELF_SHF_COMPRESSED 0x800

typedef struct s_elf_shdr { const char* name; uint32_t type; uint64_t flags, addr, offset, size; uint32_t link; uint64_t entsize; } s_elf_shdr;

typedef struct s_elf
{
	const unsigned char* file; size_t len;
	int is64, machine;
	s_elf_shdr* sh; int sh_count;
} s_elf;

static int s_elf_parse(s_elf* e, const unsigned char* file, size_t len, sym_log_fn log, void* udata)
{
	memset(e, 0, sizeof(*e));
	e->file = file; e->len = len;
	s_dw_cur c = s_dw_cur_make(file, len);
	if (!s_dw_left(&c, 64) || memcmp(file, "\x7f" "ELF", 4) != 0) { sym_logf(log, udata, "elf: not an ELF file"); return 0; }
	if (file[5] != 1) { sym_logf(log, udata, "elf: big-endian files are not supported"); return 0; }
	e->is64 = file[4] == 2;
	s_dw_skip(&c, 16);
	s_dw_u16(&c); /* type */
	e->machine = s_dw_u16(&c);
	s_dw_u32(&c); /* version */
	uint64_t shoff;
	uint16_t shentsize, shnum, shstrndx;
	if (e->is64) { s_dw_u64(&c); s_dw_u64(&c); shoff = s_dw_u64(&c); s_dw_u32(&c); s_dw_u16(&c); s_dw_u16(&c); s_dw_u16(&c); shentsize = s_dw_u16(&c); shnum = s_dw_u16(&c); shstrndx = s_dw_u16(&c); }
	else { s_dw_u32(&c); s_dw_u32(&c); shoff = s_dw_u32(&c); s_dw_u32(&c); s_dw_u16(&c); s_dw_u16(&c); s_dw_u16(&c); shentsize = s_dw_u16(&c); shnum = s_dw_u16(&c); shstrndx = s_dw_u16(&c); }
	if (!c.ok || shnum == 0 || shentsize < (e->is64 ? 64u : 40u) || shoff > len || (uint64_t)shentsize * shnum > len - shoff) { sym_logf(log, udata, "elf: section table out of bounds"); return 0; }
	e->sh = (s_elf_shdr*)calloc(shnum, sizeof(s_elf_shdr));
	if (!e->sh) return 0;
	e->sh_count = shnum;
	for (int i = 0; i < shnum; ++i) {
		s_dw_cur h = s_dw_cur_make(file + shoff + (uint64_t)i * shentsize, shentsize);
		uint32_t name_off = s_dw_u32(&h);
		s_elf_shdr* s = &e->sh[i];
		s->type = s_dw_u32(&h);
		if (e->is64) { s->flags = s_dw_u64(&h); s->addr = s_dw_u64(&h); s->offset = s_dw_u64(&h); s->size = s_dw_u64(&h); s->link = s_dw_u32(&h); s_dw_u32(&h); s_dw_u64(&h); s->entsize = s_dw_u64(&h); }
		else { s->flags = s_dw_u32(&h); s->addr = s_dw_u32(&h); s->offset = s_dw_u32(&h); s->size = s_dw_u32(&h); s->link = s_dw_u32(&h); s_dw_u32(&h); s_dw_u32(&h); s->entsize = s_dw_u32(&h); }
		s->name = "";
		(void)name_off;
		if (!h.ok) { free(e->sh); e->sh = NULL; return 0; }
		/* Names resolve after the string table is known. */
		s->name = (const char*)(uintptr_t)name_off; /* temporarily the offset */
	}
	if (shstrndx < shnum && e->sh[shstrndx].offset < len) {
		const s_elf_shdr* st = &e->sh[shstrndx];
		s_dw_sec strs = { file + st->offset, st->size > len - st->offset ? (size_t)(len - st->offset) : (size_t)st->size };
		for (int i = 0; i < shnum; ++i) {
			const char* n = s_dw_str_at(&strs, (uint64_t)(uintptr_t)e->sh[i].name);
			e->sh[i].name = n ? n : "";
		}
	} else {
		for (int i = 0; i < shnum; ++i) e->sh[i].name = "";
	}
	return 1;
}

static s_dw_sec s_elf_section(const s_elf* e, const char* name, sym_log_fn log, void* udata)
{
	s_dw_sec sec = { NULL, 0 };
	for (int i = 0; i < e->sh_count; ++i) {
		const s_elf_shdr* s = &e->sh[i];
		if (strcmp(s->name, name) != 0 || s->type == 8 /* NOBITS */) continue;
		if (s->flags & S_ELF_SHF_COMPRESSED) { sym_logf(log, udata, "elf: %s is compressed (SHF_COMPRESSED), not supported", name); return sec; }
		if (s->offset > e->len || s->size > e->len - s->offset) return sec;
		sec.data = e->file + s->offset;
		sec.len = (size_t)s->size;
		return sec;
	}
	return sec;
}

static int s_elf_build_id(const s_elf* e, unsigned char* id, int* id_len)
{
	for (int i = 0; i < e->sh_count; ++i) {
		const s_elf_shdr* s = &e->sh[i];
		if (s->type != S_ELF_SHT_NOTE || s->offset > e->len || s->size > e->len - s->offset) continue;
		s_dw_cur c = s_dw_cur_make(e->file + s->offset, (size_t)s->size);
		while (s_dw_left(&c, 12)) {
			uint32_t namesz = s_dw_u32(&c), descsz = s_dw_u32(&c), type = s_dw_u32(&c);
			uint32_t name_pad = (namesz + 3) & ~3u, desc_pad = (descsz + 3) & ~3u;
			if (!s_dw_left(&c, name_pad + desc_pad)) break;
			if (type == 3 && namesz == 4 && memcmp(c.p, "GNU", 4) == 0 && descsz > 0 && descsz <= 20) {
				memcpy(id, c.p + name_pad, descsz);
				*id_len = (int)descsz;
				return 1;
			}
			s_dw_skip(&c, name_pad + desc_pad);
		}
	}
	return 0;
}

static int s_elf_arch(int machine)
{
	switch (machine) {
	case 62: return 1;
	case 183: return 2;
	case 3: return 3;
	default: return 0;
	}
}

static void s_elf_symtab_functions(const s_elf* e, sym_sink* sink)
{
	for (int i = 0; i < e->sh_count; ++i) {
		const s_elf_shdr* s = &e->sh[i];
		if ((s->type != S_ELF_SHT_SYMTAB && s->type != 11 /* DYNSYM */) || s->link >= (uint32_t)e->sh_count) continue;
		const s_elf_shdr* st = &e->sh[s->link];
		if (s->offset > e->len || s->size > e->len - s->offset || st->offset > e->len || st->size > e->len - st->offset) continue;
		s_dw_sec strs = { e->file + st->offset, (size_t)st->size };
		size_t ent = e->is64 ? 24 : 16;
		uint64_t count = s->size / ent;
		for (uint64_t k = 0; k < count; ++k) {
			s_dw_cur c = s_dw_cur_make(e->file + s->offset + k * ent, ent);
			uint32_t name = s_dw_u32(&c);
			uint8_t info; uint64_t value, size;
			if (e->is64) { info = s_dw_u8(&c); s_dw_u8(&c); s_dw_u16(&c); value = s_dw_u64(&c); size = s_dw_u64(&c); }
			else { value = s_dw_u32(&c); size = s_dw_u32(&c); info = s_dw_u8(&c); }
			if (!c.ok || (info & 0xf) != 2 /* STT_FUNC */ || size == 0) continue;
			const char* n = s_dw_str_at(&strs, name);
			if (!n || !n[0]) continue;
			sym_sink_function(sink, value, size > 0xffffffffu ? 0xffffffffu : (uint32_t)size, n);
		}
		if (s->type == S_ELF_SHT_SYMTAB) return; /* .symtab is complete; .dynsym only when there is no .symtab */
	}
}

static void s_elf_sections(const s_elf* e, s_dw_sections* d, sym_log_fn log, void* udata)
{
	memset(d, 0, sizeof(*d));
	d->info = s_elf_section(e, ".debug_info", log, udata);
	d->abbrev = s_elf_section(e, ".debug_abbrev", log, udata);
	d->str = s_elf_section(e, ".debug_str", log, udata);
	d->line = s_elf_section(e, ".debug_line", log, udata);
	d->line_str = s_elf_section(e, ".debug_line_str", log, udata);
	d->str_offsets = s_elf_section(e, ".debug_str_offsets", log, udata);
	d->addr = s_elf_section(e, ".debug_addr", log, udata);
	d->ranges = s_elf_section(e, ".debug_ranges", log, udata);
	d->rnglists = s_elf_section(e, ".debug_rnglists", log, udata);
	d->aranges = s_elf_section(e, ".debug_aranges", log, udata);
	d->types = s_elf_section(e, ".debug_types", log, udata);
}

/* The .gnu_debuglink target beside the binary, loaded; NULL when there is none. */
static unsigned char* s_elf_debuglink(const s_elf* e, const char* path, size_t* out_len, sym_log_fn log, void* udata)
{
	s_dw_sec link = s_elf_section(e, ".gnu_debuglink", log, udata);
	if (!link.data) return NULL;
	s_dw_cur c = s_dw_cur_make(link.data, link.len);
	const char* name = s_dw_cstr(&c);
	if (!name || !name[0]) return NULL;
	char full[2048];
	const char* slash = path ? strrchr(path, '/') : NULL;
	const char* bslash = path ? strrchr(path, '\\') : NULL;
	if (bslash && (!slash || bslash > slash)) slash = bslash;
	size_t dirlen = slash ? (size_t)(slash - path + 1) : 0;
	const char* subdirs[] = { "", ".debug/" };
	for (int i = 0; i < 2; ++i) {
		if (dirlen + strlen(subdirs[i]) + strlen(name) + 1 >= sizeof(full)) continue;
		memcpy(full, path, dirlen);
		full[dirlen] = 0;
		strcat(full, subdirs[i]);
		strcat(full, name);
		unsigned char* buf = s_dw_read_file(full, out_len);
		if (buf) { sym_logf(log, udata, "elf: debug info from %s", full); return buf; }
	}
	sym_logf(log, udata, "elf: .gnu_debuglink names %s, not found beside the binary", name);
	return NULL;
}

bool sym_build_elf(const unsigned char* file, size_t len, const char* path, sym_sink* sink, sym_log_fn log, void* udata)
{
	s_elf e;
	if (!s_elf_parse(&e, file, len, log, udata)) return false;
	unsigned char id[20];
	int id_len = 0;
	if (!s_elf_build_id(&e, id, &id_len)) sym_logf(log, udata, "elf: no .note.gnu.build-id; the table carries no build id");
	sym_sink_module(sink, s_elf_arch(e.machine), id, id_len);
	s_dw_sections d;
	s_elf_sections(&e, &d, log, udata);
	unsigned char* linked = NULL;
	s_elf le;
	memset(&le, 0, sizeof(le));
	if (!d.info.data) {
		size_t linked_len = 0;
		linked = s_elf_debuglink(&e, path, &linked_len, log, udata);
		if (linked && s_elf_parse(&le, linked, linked_len, log, udata)) s_elf_sections(&le, &d, log, udata);
	}
	int ok = 0;
	if (d.info.data) ok = s_dw_read(&d, NULL, sink, log, udata);
	else sym_logf(log, udata, "elf: no DWARF; functions from the symbol table only");
	s_elf_symtab_functions(&e, sink);
	if (le.sh) { s_elf_symtab_functions(&le, sink); free(le.sh); }
	free(linked);
	free(e.sh);
	return ok || true;
}

/* ---------------------------------------------------------------------------------------------- */
/* Mach-O. */

#define S_MACHO_MH_MAGIC_64 0xfeedfacfu
#define S_MACHO_FAT_MAGIC   0xcafebabeu
#define S_MACHO_FAT_MAGIC_64 0xcafebabfu
#define S_MACHO_LC_SEGMENT_64 0x19
#define S_MACHO_LC_SYMTAB 0x2
#define S_MACHO_LC_UUID 0x1b
#define S_MACHO_CPU_X86_64 0x01000007
#define S_MACHO_CPU_ARM64  0x0100000c
#define S_MACHO_MH_OBJECT 1

typedef struct s_macho_section { char segname[17], sectname[17]; uint64_t addr, size; uint32_t offset, reloff, nreloc; } s_macho_section;
typedef struct s_macho_sym { uint32_t strx; uint8_t type, sect; uint16_t desc; uint64_t value; } s_macho_sym;

typedef struct s_macho
{
	const unsigned char* file; size_t len;
	uint32_t cputype, filetype;
	s_macho_section* sects; int sect_count;
	s_macho_sym* syms; int sym_count;
	const unsigned char* strtab; size_t strtab_len;
	unsigned char uuid[16]; int has_uuid;
} s_macho;

/* Picks a slice of a fat file for the host's architecture (else the first); returns the slice. */
static const unsigned char* s_macho_slice(const unsigned char* file, size_t len, size_t* slice_len, sym_log_fn log, void* udata)
{
	s_dw_cur c = s_dw_cur_make(file, len);
	uint32_t magic = s_dw_u32be(&c);
	if (magic != S_MACHO_FAT_MAGIC && magic != S_MACHO_FAT_MAGIC_64) { *slice_len = len; return file; }
	int fat64 = magic == S_MACHO_FAT_MAGIC_64;
	uint32_t n = s_dw_u32be(&c);
	if (!c.ok || n == 0 || n > 64) return NULL;
#if defined(__aarch64__) || defined(_M_ARM64)
	uint32_t want = S_MACHO_CPU_ARM64;
#else
	uint32_t want = S_MACHO_CPU_X86_64;
#endif
	uint64_t first_off = 0, first_size = 0, pick_off = 0, pick_size = 0;
	for (uint32_t i = 0; i < n; ++i) {
		uint32_t cputype = s_dw_u32be(&c);
		s_dw_u32be(&c);
		uint64_t off = fat64 ? s_dw_u64be(&c) : s_dw_u32be(&c);
		uint64_t size = fat64 ? s_dw_u64be(&c) : s_dw_u32be(&c);
		s_dw_u32be(&c);
		if (fat64) s_dw_u32be(&c);
		if (!c.ok) return NULL;
		if (i == 0) { first_off = off; first_size = size; }
		if (cputype == want && !pick_size) { pick_off = off; pick_size = size; }
	}
	if (!pick_size) { pick_off = first_off; pick_size = first_size; sym_logf(log, udata, "macho: fat file without a slice for this host; the first slice is used"); }
	if (pick_off > len || pick_size > len - pick_off) return NULL;
	*slice_len = (size_t)pick_size;
	return file + pick_off;
}

static int s_macho_parse(s_macho* m, const unsigned char* file, size_t len, sym_log_fn log, void* udata)
{
	memset(m, 0, sizeof(*m));
	m->file = file; m->len = len;
	s_dw_cur c = s_dw_cur_make(file, len);
	uint32_t magic = s_dw_u32(&c);
	if (magic != S_MACHO_MH_MAGIC_64) { sym_logf(log, udata, "macho: not a 64-bit little-endian Mach-O"); return 0; }
	m->cputype = s_dw_u32(&c);
	s_dw_u32(&c);
	m->filetype = s_dw_u32(&c);
	uint32_t ncmds = s_dw_u32(&c), sizeofcmds = s_dw_u32(&c);
	s_dw_u32(&c); s_dw_u32(&c);
	if (!c.ok || ncmds > 4096 || sizeofcmds > len) return 0;
	const unsigned char* cmd = c.p;
	const unsigned char* cmds_end = c.p + sizeofcmds;
	if (cmds_end > file + len) return 0;
	for (uint32_t i = 0; i < ncmds; ++i) {
		s_dw_cur h = s_dw_cur_make(cmd, (size_t)(cmds_end - cmd));
		uint32_t kind = s_dw_u32(&h), size = s_dw_u32(&h);
		if (!h.ok || size < 8 || size > (uint32_t)(cmds_end - cmd)) return 0;
		if (kind == S_MACHO_LC_SEGMENT_64) {
			char segname[17];
			if (!s_dw_left(&h, 16)) return 0;
			memcpy(segname, h.p, 16); segname[16] = 0;
			s_dw_skip(&h, 16);
			s_dw_u64(&h); s_dw_u64(&h); s_dw_u64(&h); s_dw_u64(&h);
			s_dw_u32(&h); s_dw_u32(&h);
			uint32_t nsects = s_dw_u32(&h);
			s_dw_u32(&h);
			if (!h.ok || nsects > 4096) return 0;
			for (uint32_t k = 0; k < nsects; ++k) {
				if (!s_dw_left(&h, 80)) return 0;
				s_macho_section s;
				memcpy(s.sectname, h.p, 16); s.sectname[16] = 0;
				memcpy(s.segname, h.p + 16, 16); s.segname[16] = 0;
				s_dw_skip(&h, 32);
				s.addr = s_dw_u64(&h); s.size = s_dw_u64(&h);
				s.offset = s_dw_u32(&h); s_dw_u32(&h);
				s.reloff = s_dw_u32(&h); s.nreloc = s_dw_u32(&h);
				s_dw_u32(&h); s_dw_u32(&h); s_dw_u32(&h); s_dw_u32(&h);
				if (!h.ok) return 0;
				s_macho_section* grown = (s_macho_section*)realloc(m->sects, (size_t)(m->sect_count + 1) * sizeof(s_macho_section));
				if (!grown) return 0;
				m->sects = grown;
				m->sects[m->sect_count++] = s;
			}
		} else if (kind == S_MACHO_LC_SYMTAB) {
			uint32_t symoff = s_dw_u32(&h), nsyms = s_dw_u32(&h), stroff = s_dw_u32(&h), strsize = s_dw_u32(&h);
			if (!h.ok) return 0;
			if (symoff <= len && (uint64_t)nsyms * 16 <= len - symoff && stroff <= len && strsize <= len - stroff && nsyms < 50000000u) {
				m->syms = (s_macho_sym*)malloc((size_t)nsyms * sizeof(s_macho_sym) + 1);
				if (!m->syms) return 0;
				for (uint32_t k = 0; k < nsyms; ++k) {
					s_dw_cur e = s_dw_cur_make(file + symoff + (uint64_t)k * 16, 16);
					m->syms[k].strx = s_dw_u32(&e);
					m->syms[k].type = s_dw_u8(&e);
					m->syms[k].sect = s_dw_u8(&e);
					m->syms[k].desc = s_dw_u16(&e);
					m->syms[k].value = s_dw_u64(&e);
				}
				m->sym_count = (int)nsyms;
				m->strtab = file + stroff;
				m->strtab_len = strsize;
			}
		} else if (kind == S_MACHO_LC_UUID) {
			if (!s_dw_left(&h, 16)) return 0;
			memcpy(m->uuid, h.p, 16);
			m->has_uuid = 1;
		}
		cmd += size;
	}
	return 1;
}

static void s_macho_free(s_macho* m)
{
	free(m->sects); free(m->syms);
	m->sects = NULL; m->syms = NULL;
}

static const char* s_macho_symname(const s_macho* m, const s_macho_sym* s)
{
	s_dw_sec strs = { m->strtab, m->strtab_len };
	const char* n = s_dw_str_at(&strs, s->strx);
	return n ? n : "";
}

static const s_macho_section* s_macho_find_section(const s_macho* m, const char* seg, const char* sect)
{
	for (int i = 0; i < m->sect_count; ++i) if (strcmp(m->sects[i].segname, seg) == 0 && strcmp(m->sects[i].sectname, sect) == 0) return &m->sects[i];
	return NULL;
}

/* A __DWARF section's bytes, relocated when the file is an object (a private copy then). */
typedef struct s_macho_dwarf
{
	s_dw_sections d;
	unsigned char* copies[12]; int copy_count; /* one relocated copy per DWARF section an object may carry */
} s_macho_dwarf;

static void s_macho_dwarf_free(s_macho_dwarf* md)
{
	for (int i = 0; i < md->copy_count; ++i) free(md->copies[i]);
	md->copy_count = 0;
}

static s_dw_sec s_macho_dwarf_section(const s_macho* m, s_macho_dwarf* md, const char* sect, sym_log_fn log, void* udata)
{
	s_dw_sec out = { NULL, 0 };
	const s_macho_section* s = s_macho_find_section(m, "__DWARF", sect);
	if (!s || s->offset > m->len || s->size > m->len - s->offset) return out;
	if (m->filetype != S_MACHO_MH_OBJECT || s->nreloc == 0) { out.data = m->file + s->offset; out.len = (size_t)s->size; return out; }
	/* An object file: apply the section's relocations to a copy. Only the two unsigned kinds and the
	   subtractor pair matter in DWARF; everything else is left as stored. */
	if (md->copy_count >= 12 || s->reloff > m->len || (uint64_t)s->nreloc * 8 > m->len - s->reloff) return out;
	unsigned char* copy = (unsigned char*)malloc((size_t)s->size + 1);
	if (!copy) return out;
	memcpy(copy, m->file + s->offset, (size_t)s->size);
	md->copies[md->copy_count++] = copy;
	int pending_sub = 0;
	uint64_t sub_value = 0;
	for (uint32_t i = 0; i < s->nreloc; ++i) {
		s_dw_cur r = s_dw_cur_make(m->file + s->reloff + (uint64_t)i * 8, 8);
		uint32_t address = s_dw_u32(&r), packed = s_dw_u32(&r);
		uint32_t symbolnum = packed & 0xffffff;
		int pcrel = (packed >> 24) & 1, length = (packed >> 25) & 3, external = (packed >> 27) & 1, type = (packed >> 28) & 0xf;
		if (address & 0x80000000u) continue; /* scattered: not emitted for 64-bit targets */
		size_t width = length == 3 ? 8 : length == 2 ? 4 : 0;
		if (!width || pcrel || (uint64_t)address + width > s->size) { pending_sub = 0; continue; }
		uint64_t target = 0;
		if (external) {
			if (symbolnum >= (uint32_t)m->sym_count) { pending_sub = 0; continue; }
			target = m->syms[symbolnum].value;
		}
		int is_sub = (m->cputype == S_MACHO_CPU_X86_64 && type == 5) || (m->cputype == S_MACHO_CPU_ARM64 && type == 1);
		if (is_sub) { pending_sub = 1; sub_value = target; continue; }
		if (type != 0) { pending_sub = 0; continue; } /* X86_64_RELOC_UNSIGNED / ARM64_RELOC_UNSIGNED only */
		s_dw_cur v = s_dw_cur_make(copy + address, width);
		uint64_t stored = width == 8 ? s_dw_u64(&v) : s_dw_u32(&v);
		uint64_t value = external ? stored + target : stored;
		if (pending_sub) { value -= sub_value; pending_sub = 0; }
		for (size_t b = 0; b < width; ++b) copy[address + b] = (unsigned char)(value >> (8 * b));
	}
	(void)log; (void)udata;
	out.data = copy; out.len = (size_t)s->size;
	return out;
}

static void s_macho_dwarf_sections(const s_macho* m, s_macho_dwarf* md, sym_log_fn log, void* udata)
{
	memset(md, 0, sizeof(*md));
	md->d.info = s_macho_dwarf_section(m, md, "__debug_info", log, udata);
	md->d.abbrev = s_macho_dwarf_section(m, md, "__debug_abbrev", log, udata);
	md->d.str = s_macho_dwarf_section(m, md, "__debug_str", log, udata);
	md->d.line = s_macho_dwarf_section(m, md, "__debug_line", log, udata);
	md->d.line_str = s_macho_dwarf_section(m, md, "__debug_line_str", log, udata);
	md->d.str_offsets = s_macho_dwarf_section(m, md, "__debug_str_offs", log, udata);
	md->d.addr = s_macho_dwarf_section(m, md, "__debug_addr", log, udata);
	md->d.ranges = s_macho_dwarf_section(m, md, "__debug_ranges", log, udata);
	md->d.rnglists = s_macho_dwarf_section(m, md, "__debug_rnglists", log, udata);
	md->d.aranges = s_macho_dwarf_section(m, md, "__debug_aranges", log, udata);
	md->d.types = s_macho_dwarf_section(m, md, "__debug_types", log, udata);
}

static int s_macho_arch(uint32_t cputype)
{
	if (cputype == S_MACHO_CPU_X86_64) return 1;
	if (cputype == S_MACHO_CPU_ARM64) return 2;
	if (cputype == 7) return 3;
	return 0;
}

/* Functions from the symbol table: defined, in a section, not a stab. Sizes come from the next
   symbol in the same section. */
static void s_macho_symtab_functions(const s_macho* m, sym_sink* sink)
{
	if (!m->sym_count) return;
	int n = 0;
	s_macho_sym* sorted = (s_macho_sym*)malloc((size_t)m->sym_count * sizeof(s_macho_sym));
	if (!sorted) return;
	for (int i = 0; i < m->sym_count; ++i) {
		const s_macho_sym* s = &m->syms[i];
		if ((s->type & 0xe0) || (s->type & 0x0e) != 0x0e || s->sect == 0) continue; /* stab, or not N_SECT */
		const char* name = s_macho_symname(m, s);
		if (!name[0]) continue;
		const s_macho_section* sec = s->sect <= m->sect_count ? &m->sects[s->sect - 1] : NULL;
		if (!sec || strcmp(sec->segname, "__TEXT") != 0) continue;
		if (s->value < sec->addr || s->value >= sec->addr + sec->size) continue; /* the header symbol and friends */
		sorted[n++] = *s;
	}
	/* Insertion sort is fine for the symbol counts of a game; a huge binary makes this O(n^2), so switch to qsort. */
	for (int i = 1; i < n; ++i) { s_macho_sym k = sorted[i]; int j = i - 1; while (j >= 0 && sorted[j].value > k.value) { sorted[j + 1] = sorted[j]; --j; } sorted[j + 1] = k; }
	for (int i = 0; i < n; ++i) {
		uint64_t end;
		if (i + 1 < n && sorted[i + 1].sect == sorted[i].sect) end = sorted[i + 1].value;
		else { const s_macho_section* sec = &m->sects[sorted[i].sect - 1]; end = sec->addr + sec->size; }
		if (end <= sorted[i].value) continue;
		uint64_t size = end - sorted[i].value;
		const char* name = s_macho_symname(m, &sorted[i]);
		if (name[0] == '_') name++;
		sym_sink_function(sink, sorted[i].value, size > 0xffffffffu ? 0xffffffffu : (uint32_t)size, name);
	}
	free(sorted);
}

/* The debug map: per object file, the functions it holds with their linked addresses. */
typedef struct s_macho_map_fn { const char* name; uint64_t linked, size; } s_macho_map_fn;
typedef struct s_macho_map_obj { const char* path; uint64_t mtime; s_macho_map_fn* fns; int fn_count, fn_cap; } s_macho_map_obj;
typedef struct s_macho_map { s_macho_map_obj* objs; int obj_count, obj_cap; } s_macho_map;

#define S_MACHO_N_STAB 0xe0
#define S_MACHO_N_FUN  0x24
#define S_MACHO_N_SO   0x64
#define S_MACHO_N_OSO  0x66

static int s_macho_debug_map(const s_macho* m, s_macho_map* map)
{
	memset(map, 0, sizeof(*map));
	s_macho_map_obj* cur = NULL;
	for (int i = 0; i < m->sym_count; ++i) {
		const s_macho_sym* s = &m->syms[i];
		if (!(s->type & S_MACHO_N_STAB)) continue;
		if (s->type == S_MACHO_N_OSO) {
			if (!s_dw_grow((void**)&map->objs, &map->obj_cap, map->obj_count + 1, sizeof(s_macho_map_obj))) return 0;
			cur = &map->objs[map->obj_count++];
			memset(cur, 0, sizeof(*cur));
			cur->path = s_macho_symname(m, s);
			cur->mtime = s->value;
		} else if (s->type == S_MACHO_N_SO) {
			if (!s_macho_symname(m, s)[0]) cur = NULL;
		} else if (s->type == S_MACHO_N_FUN && cur) {
			const char* name = s_macho_symname(m, s);
			if (name[0]) {
				if (!s_dw_grow((void**)&cur->fns, &cur->fn_cap, cur->fn_count + 1, sizeof(s_macho_map_fn))) return 0;
				cur->fns[cur->fn_count].name = name;
				cur->fns[cur->fn_count].linked = s->value;
				cur->fns[cur->fn_count].size = 0;
				cur->fn_count++;
			} else if (cur->fn_count) {
				cur->fns[cur->fn_count - 1].size = s->value;
			}
		}
	}
	return 1;
}

static void s_macho_map_free(s_macho_map* map)
{
	for (int i = 0; i < map->obj_count; ++i) free(map->objs[i].fns);
	free(map->objs);
}

/* The rebase for one object: object address ranges of its functions onto the linked ones. */
typedef struct s_macho_span { uint64_t obj, size, linked; } s_macho_span;
typedef struct s_macho_rebase { s_macho_span* spans; int count; } s_macho_rebase;

static int s_macho_rebase_fn(void* ctx, uint64_t addr, uint64_t* out, uint64_t* span_end)
{
	const s_macho_rebase* r = (const s_macho_rebase*)ctx;
	int lo = 0, hi = r->count - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		const s_macho_span* s = &r->spans[mid];
		if (addr < s->obj) hi = mid - 1;
		else if (addr > s->obj + s->size) lo = mid + 1;
		else {
			/* An address at a span's end belongs to the next span when one starts there. */
			if (addr == s->obj + s->size && mid + 1 < r->count && r->spans[mid + 1].obj == addr) s = &r->spans[mid + 1];
			*out = s->linked + (addr - s->obj);
			*span_end = s->linked + s->size;
			return 1;
		}
	}
	return 0;
}

static int s_macho_span_less(const void* a, const void* b)
{
	const s_macho_span* x = (const s_macho_span*)a;
	const s_macho_span* y = (const s_macho_span*)b;
	return x->obj < y->obj ? -1 : x->obj > y->obj ? 1 : 0;
}

/* Reads one object file of the debug map into the sink, rebased. */
/* An archive member's bytes, by name, out of a `!<arch>` file: BSD long names (`#1/<len>`, the
   name leading the data) and GNU ones (`/<offset>` into the `//` table); `/`, `//` and `__.SYMDEF`
   members are tables, never objects. A member whose mtime disagrees with the debug map's is used
   anyway, logged: the map is the authority on which member was linked. */
static unsigned char* s_macho_archive_member(const char* archive, const char* member, uint64_t mtime, size_t* out_len, sym_log_fn log, void* udata)
{
	size_t len = 0;
	unsigned char* ar = s_dw_read_file(archive, &len);
	unsigned char* out = NULL;
	if (!ar) { sym_logf(log, udata, "macho: archive %s not found", archive); return NULL; }
	if (len < 8 || memcmp(ar, "!<arch>\n", 8) != 0) { sym_logf(log, udata, "macho: %s is not an archive", archive); free(ar); return NULL; }
	const char* longnames = NULL; size_t longnames_len = 0;
	size_t off = 8;
	while (off + 60 <= len) {
		const char* h = (const char*)ar + off;
		char name[17], sizebuf[11], mtimebuf[13];
		memcpy(name, h, 16); name[16] = 0;
		memcpy(mtimebuf, h + 16, 12); mtimebuf[12] = 0;
		memcpy(sizebuf, h + 48, 10); sizebuf[10] = 0;
		if (h[58] != '`' || h[59] != '\n') break;
		uint64_t size = strtoull(sizebuf, NULL, 10);
		uint64_t member_mtime = strtoull(mtimebuf, NULL, 10);
		size_t data = off + 60;
		if (size > len - data) break;
		const unsigned char* bytes = ar + data;
		size_t bytes_len = (size_t)size;
		char resolved[1024];
		resolved[0] = 0;
		int n = 15;
		while (n >= 0 && name[n] == ' ') name[n--] = 0;
		if (strncmp(name, "#1/", 3) == 0) {
			size_t nl = (size_t)strtoull(name + 3, NULL, 10);
			if (nl > bytes_len || nl >= sizeof(resolved)) break;
			memcpy(resolved, bytes, nl); resolved[nl] = 0;
			for (size_t k = nl; k > 0 && resolved[k - 1] == 0; --k) resolved[k - 1] = 0; /* NUL padding */
			bytes += nl; bytes_len -= nl;
		} else if (strcmp(name, "//") == 0) {
			longnames = (const char*)bytes; longnames_len = bytes_len;
		} else if (name[0] == '/' && name[1] >= '0' && name[1] <= '9' && longnames) {
			size_t so = (size_t)strtoull(name + 1, NULL, 10);
			size_t k = 0;
			while (so + k < longnames_len && longnames[so + k] != '/' && longnames[so + k] != '\n' && k + 1 < sizeof(resolved)) { resolved[k] = longnames[so + k]; ++k; }
			resolved[k] = 0;
		} else {
			size_t nl = strlen(name);
			if (nl > 0 && name[nl - 1] == '/' && nl > 1) name[nl - 1] = 0; /* GNU short names end in `/` */
			strcpy(resolved, name);
		}
		if (resolved[0] && strcmp(resolved, member) == 0 && strcmp(resolved, "/") != 0 && strncmp(resolved, "__.SYMDEF", 9) != 0) {
			if (mtime && member_mtime && mtime != member_mtime) sym_logf(log, udata, "macho: %s(%s): the archive member's timestamp differs from the debug map's; used anyway", archive, member);
			out = (unsigned char*)malloc(bytes_len + 1);
			if (out) { memcpy(out, bytes, bytes_len); out[bytes_len] = 0; *out_len = bytes_len; }
			break;
		}
		off = data + (size_t)size + ((size_t)size & 1);
	}
	if (!out) sym_logf(log, udata, "macho: %s has no member %s", archive, member);
	free(ar);
	return out;
}

static int s_macho_read_object(const s_macho_map_obj* obj, sym_sink* sink, sym_log_fn log, void* udata)
{
	size_t len = 0;
	unsigned char* file = NULL;
	const char* paren = strchr(obj->path, '(');
	if (paren && obj->path[strlen(obj->path) - 1] == ')') {
		char archive[1024], member[256];
		size_t al = (size_t)(paren - obj->path), ml = strlen(paren) - 2;
		if (al >= sizeof(archive) || ml >= sizeof(member)) { sym_logf(log, udata, "macho: %s: path too long", obj->path); return 0; }
		memcpy(archive, obj->path, al); archive[al] = 0;
		memcpy(member, paren + 1, ml); member[ml] = 0;
		file = s_macho_archive_member(archive, member, obj->mtime, &len, log, udata);
	} else {
		file = s_dw_read_file(obj->path, &len);
	}
	if (!file) { sym_logf(log, udata, "macho: object %s not found; its functions are raw", obj->path); return 0; }
	s_macho m;
	int ok = 0;
	if (s_macho_parse(&m, file, len, log, udata) && m.filetype == S_MACHO_MH_OBJECT) {
		s_macho_rebase rb;
		rb.spans = (s_macho_span*)malloc((size_t)(obj->fn_count + 1) * sizeof(s_macho_span));
		rb.count = 0;
		if (rb.spans) {
			for (int i = 0; i < obj->fn_count; ++i) {
				const s_macho_map_fn* f = &obj->fns[i];
				for (int k = 0; k < m.sym_count; ++k) {
					const s_macho_sym* s = &m.syms[k];
					if ((s->type & S_MACHO_N_STAB) || (s->type & 0x0e) != 0x0e || s->sect == 0) continue;
					if (strcmp(s_macho_symname(&m, s), f->name) != 0) continue;
					rb.spans[rb.count].obj = s->value;
					rb.spans[rb.count].size = f->size;
					rb.spans[rb.count].linked = f->linked;
					rb.count++;
					break;
				}
			}
			qsort(rb.spans, (size_t)rb.count, sizeof(s_macho_span), s_macho_span_less);
			s_macho_dwarf md;
			s_macho_dwarf_sections(&m, &md, log, udata);
			if (md.d.info.data) {
				s_dw_rebase r = { s_macho_rebase_fn, &rb };
				ok = s_dw_read(&md.d, &r, sink, log, udata);
			} else {
				sym_logf(log, udata, "macho: object %s has no DWARF", obj->path);
			}
			s_macho_dwarf_free(&md);
			free(rb.spans);
		}
	} else {
		sym_logf(log, udata, "macho: %s is not an object file", obj->path);
	}
	s_macho_free(&m);
	free(file);
	return ok;
}

static const char* s_macho_basename(const char* path)
{
	const char* slash = strrchr(path, '/');
	const char* bslash = strrchr(path, '\\');
	if (bslash && (!slash || bslash > slash)) slash = bslash;
	return slash ? slash + 1 : path;
}

/* The dSYM's DWARF file: debug_path as given (a file, or a bundle to look inside), else <path>.dSYM beside the binary. */
static unsigned char* s_macho_load_dsym(const char* path, const char* debug_path, size_t* len, sym_log_fn log, void* udata)
{
	char full[2048];
	const char* base = s_macho_basename(path);
	unsigned char* f = NULL;
	if (debug_path) {
		f = s_dw_read_file(debug_path, len);
		if (f) return f;
		snprintf(full, sizeof(full), "%s/Contents/Resources/DWARF/%s", debug_path, base);
		f = s_dw_read_file(full, len);
		if (f) return f;
		sym_logf(log, udata, "macho: no DWARF file at %s", debug_path);
		return NULL;
	}
	snprintf(full, sizeof(full), "%s.dSYM/Contents/Resources/DWARF/%s", path, base);
	f = s_dw_read_file(full, len);
	if (f) sym_logf(log, udata, "macho: debug info from %s", full);
	return f;
}

bool sym_build_macho(const unsigned char* file, size_t len, const char* path, const char* debug_path, sym_sink* sink, sym_log_fn log, void* udata)
{
	size_t slice_len = 0;
	const unsigned char* slice = s_macho_slice(file, len, &slice_len, log, udata);
	if (!slice) { sym_logf(log, udata, "macho: damaged fat header"); return false; }
	s_macho m;
	if (!s_macho_parse(&m, slice, slice_len, log, udata)) { s_macho_free(&m); return false; }
	sym_sink_module(sink, s_macho_arch(m.cputype), m.uuid, m.has_uuid ? 16 : 0);
	if (!m.has_uuid) sym_logf(log, udata, "macho: no LC_UUID; the table carries no build id");
	int ok = 0;
	s_macho_dwarf md;
	s_macho_dwarf_sections(&m, &md, log, udata);
	if (md.d.info.data) {
		ok = s_dw_read(&md.d, NULL, sink, log, udata);
	} else {
		size_t dlen = 0;
		unsigned char* dfile = s_macho_load_dsym(path, debug_path, &dlen, log, udata);
		if (dfile) {
			size_t dslice_len = 0;
			const unsigned char* dslice = s_macho_slice(dfile, dlen, &dslice_len, log, udata);
			s_macho dm;
			if (dslice && s_macho_parse(&dm, dslice, dslice_len, log, udata)) {
				if (dm.has_uuid && m.has_uuid && memcmp(dm.uuid, m.uuid, 16) != 0) {
					sym_logf(log, udata, "macho: the dSYM's UUID does not match the binary; ignored");
				} else {
					s_macho_dwarf dmd;
					s_macho_dwarf_sections(&dm, &dmd, log, udata);
					if (dmd.d.info.data) ok = s_dw_read(&dmd.d, NULL, sink, log, udata);
					else sym_logf(log, udata, "macho: the dSYM holds no DWARF");
					s_macho_dwarf_free(&dmd);
				}
			}
			s_macho_free(&dm);
			free(dfile);
		} else if (m.filetype != S_MACHO_MH_OBJECT) {
			s_macho_map map;
			if (s_macho_debug_map(&m, &map) && map.obj_count) {
				sym_logf(log, udata, "macho: no dSYM; reading %d object files through the debug map", map.obj_count);
				int read = 0;
				for (int i = 0; i < map.obj_count; ++i) read += s_macho_read_object(&map.objs[i], sink, log, udata);
				ok = read > 0;
			} else {
				sym_logf(log, udata, "macho: no DWARF, no dSYM, no debug map; functions from the symbol table only");
			}
			s_macho_map_free(&map);
		}
	}
	s_macho_dwarf_free(&md);
	s_macho_symtab_functions(&m, sink);
	s_macho_free(&m);
	(void)ok;
	return true;
}

/* ---------------------------------------------------------------------------------------------- */
/* PE carrying DWARF (MinGW). Addresses in the DWARF are virtual; the table wants RVAs. */

typedef struct s_pe_dwarf_ctx { uint64_t image_base; } s_pe_dwarf_ctx;

static int s_pe_dwarf_rebase(void* ctx, uint64_t addr, uint64_t* out, uint64_t* span_end)
{
	const s_pe_dwarf_ctx* p = (const s_pe_dwarf_ctx*)ctx;
	(void)span_end;
	if (addr < p->image_base) return 0;
	*out = addr - p->image_base;
	return 1;
}

bool sym_build_pe_dwarf(const unsigned char* file, size_t len, sym_sink* sink, sym_log_fn log, void* udata)
{
	s_dw_cur c = s_dw_cur_make(file, len);
	if (!s_dw_left(&c, 64) || file[0] != 'M' || file[1] != 'Z') return false;
	s_dw_skip(&c, 60);
	uint32_t pe_off = s_dw_u32(&c);
	if (!c.ok || pe_off > len - 24) return false;
	c = s_dw_cur_make(file + pe_off, len - pe_off);
	if (s_dw_u32(&c) != 0x00004550u) return false;
	uint16_t machine = s_dw_u16(&c), nsections = s_dw_u16(&c);
	s_dw_u32(&c);
	uint32_t symtab = s_dw_u32(&c), nsyms = s_dw_u32(&c);
	uint16_t opt_size = s_dw_u16(&c);
	s_dw_u16(&c);
	const unsigned char* opt = c.p;
	uint64_t image_base = 0;
	if (opt_size >= 28) {
		s_dw_cur o = s_dw_cur_make(opt, opt_size);
		uint16_t magic = s_dw_u16(&o);
		if (magic == 0x20b && opt_size >= 32) { s_dw_skip(&o, 22); image_base = s_dw_u64(&o); }
		else if (magic == 0x10b) { s_dw_skip(&o, 26); image_base = s_dw_u32(&o); }
	}
	s_dw_skip(&c, opt_size);
	if (!c.ok || !s_dw_left(&c, (size_t)nsections * 40)) return false;
	s_dw_sec strtab = { NULL, 0 };
	if (symtab && symtab <= len && (uint64_t)nsyms * 18 <= len - symtab) {
		uint64_t at = symtab + (uint64_t)nsyms * 18;
		if (at + 4 <= len) {
			s_dw_cur t = s_dw_cur_make(file + at, len - (size_t)at);
			uint32_t size = s_dw_u32(&t);
			if (size >= 4 && size <= len - at) { strtab.data = file + at; strtab.len = size; }
		}
	}
	s_dw_sections d;
	memset(&d, 0, sizeof(d));
	for (int i = 0; i < nsections; ++i) {
		s_dw_cur h = s_dw_cur_make(c.p + (size_t)i * 40, 40);
		char name[9];
		memcpy(name, h.p, 8); name[8] = 0;
		s_dw_skip(&h, 8);
		s_dw_u32(&h); /* virtual size */
		s_dw_u32(&h); /* virtual address */
		uint32_t raw_size = s_dw_u32(&h), raw_ptr = s_dw_u32(&h);
		if (!h.ok) return false;
		const char* full = name;
		if (name[0] == '/' && strtab.data) {
			uint64_t off = (uint64_t)atoi(name + 1);
			const char* n = s_dw_str_at(&strtab, off);
			if (n) full = n;
		}
		if (raw_ptr > len || raw_size > len - raw_ptr) continue;
		s_dw_sec sec = { file + raw_ptr, raw_size };
		if (strcmp(full, ".debug_info") == 0) d.info = sec;
		else if (strcmp(full, ".debug_abbrev") == 0) d.abbrev = sec;
		else if (strcmp(full, ".debug_str") == 0) d.str = sec;
		else if (strcmp(full, ".debug_line") == 0) d.line = sec;
		else if (strcmp(full, ".debug_line_str") == 0) d.line_str = sec;
		else if (strcmp(full, ".debug_str_offsets") == 0) d.str_offsets = sec;
		else if (strcmp(full, ".debug_addr") == 0) d.addr = sec;
		else if (strcmp(full, ".debug_ranges") == 0) d.ranges = sec;
		else if (strcmp(full, ".debug_rnglists") == 0) d.rnglists = sec;
		else if (strcmp(full, ".debug_aranges") == 0) d.aranges = sec;
		else if (strcmp(full, ".debug_types") == 0) d.types = sec;
	}
	if (!d.info.data) return false;
	int arch = machine == 0x8664 ? 1 : machine == 0xaa64 ? 2 : machine == 0x14c ? 3 : 0;
	(void)arch;
	s_pe_dwarf_ctx pc = { image_base };
	s_dw_rebase rb = { s_pe_dwarf_rebase, &pc };
	return s_dw_read(&d, &rb, sink, log, udata) ? true : false;
}

bool sym_build(const char* binary_path, const char* debug_path, sym_table* out, sym_log_fn log, void* udata)
{
	size_t len;
	unsigned char* file = s_read_file(binary_path, &len);
	sym_sink* sink;
	bool ok = false;
	memset(out, 0, sizeof(*out));
	if (!file) { sym_logf(log, udata, "cute_sym: cannot read %s", binary_path); return false; }
	sink = s_sink_make();
	if (!sink) { free(file); return false; }
	if (len >= 2 && file[0] == 'M' && file[1] == 'Z') {
		s_pe pe;
		if (s_pe_parse(&pe, file, len, log, udata)) {
			unsigned char id[20];
			s_pe_build_id(&pe, id);
			sym_sink_module(sink, pe.arch, id, pe.has_cv ? 20 : 0);
			ok = pe.has_cv && s_pdb_build(&pe, binary_path, debug_path, sink, log, udata);
			if (!ok) ok = sym_build_pe_dwarf(file, len, sink, log, udata);
			if (!ok) sym_logf(log, udata, "cute_sym: %s: no usable debug information", binary_path);
		} else {
			sym_logf(log, udata, "cute_sym: %s: not a PE image", binary_path);
		}
	} else if (len >= 4 && memcmp(file, "\x7F" "ELF", 4) == 0) {
		ok = sym_build_elf(file, len, binary_path, sink, log, udata);
	} else if (len >= 4 && (s_rd32(file) == 0xFEEDFACFu || s_rd32(file) == 0xFEEDFACEu || s_rd32(file) == 0xCAFEBABEu || s_rd32(file) == 0xBEBAFECAu || s_rd32(file) == 0xCFFAEDFEu)) {
		ok = sym_build_macho(file, len, binary_path, debug_path, sink, log, udata);
	} else {
		sym_logf(log, udata, "cute_sym: %s: not a PE, ELF or Mach-O file", binary_path);
	}
	free(file);
	if (!ok) { s_sink_free(sink); return false; }
	return s_sink_finish(sink, out, log, udata);
}

//--------------------------------------------------------------------------------------------------
// JSON, enough for the report: a tree with ordered keys, read tolerantly, written back pretty.

enum { S_JNULL, S_JBOOL, S_JNUM, S_JSTR, S_JARR, S_JOBJ };

typedef struct s_jval {
	int type;
	char* text;                 // S_JSTR: the string; S_JNUM / S_JBOOL: the literal as written.
	struct s_jval** items;      // S_JARR and S_JOBJ.
	char** keys;                // S_JOBJ.
	size_t count, cap;
} s_jval;

static s_jval* s_jnew(int type)
{
	s_jval* v = (s_jval*)calloc(1, sizeof(s_jval));
	if (v) v->type = type;
	return v;
}

static void s_jfree(s_jval* v)
{
	size_t i;
	if (!v) return;
	free(v->text);
	for (i = 0; i < v->count; ++i) { s_jfree(v->items[i]); if (v->keys) free(v->keys[i]); }
	free(v->items); free(v->keys);
	free(v);
}

static char* s_jstrdup(const char* s)
{
	size_t n = strlen(s) + 1;
	char* d = (char*)malloc(n);
	if (d) memcpy(d, s, n);
	return d;
}

static s_jval* s_jstr(const char* s) { s_jval* v = s_jnew(S_JSTR); if (v) v->text = s_jstrdup(s); return v; }
static s_jval* s_jnum(long long n) { char b[32]; s_jval* v = s_jnew(S_JNUM); snprintf(b, sizeof(b), "%lld", n); if (v) v->text = s_jstrdup(b); return v; }

static bool s_jpush(s_jval* arr, s_jval* item)
{
	arr->items = (s_jval**)s_grow(arr->items, &arr->cap, arr->count, sizeof(s_jval*));
	if (!arr->items) return false;
	arr->items[arr->count++] = item;
	return true;
}

static s_jval* s_jget(const s_jval* obj, const char* key)
{
	size_t i;
	if (!obj || obj->type != S_JOBJ) return NULL;
	for (i = 0; i < obj->count; ++i) if (strcmp(obj->keys[i], key) == 0) return obj->items[i];
	return NULL;
}

static const char* s_jstring(const s_jval* obj, const char* key)
{
	const s_jval* v = s_jget(obj, key);
	return v && v->type == S_JSTR ? v->text : NULL;
}

static long long s_jint(const s_jval* obj, const char* key, long long dflt)
{
	const s_jval* v = s_jget(obj, key);
	return v && v->type == S_JNUM ? strtoll(v->text, NULL, 10) : dflt;
}

// Sets a key, replacing in place or appending; takes ownership of `val`.
static bool s_jset(s_jval* obj, const char* key, s_jval* val)
{
	size_t i;
	if (!val) return false;
	for (i = 0; i < obj->count; ++i) {
		if (strcmp(obj->keys[i], key) == 0) { s_jfree(obj->items[i]); obj->items[i] = val; return true; }
	}
	obj->items = (s_jval**)s_grow(obj->items, &obj->cap, obj->count, sizeof(s_jval*));
	{
		size_t kcap = obj->cap;
		char** nk = (char**)realloc(obj->keys, kcap * sizeof(char*));
		if (!obj->items || !nk) { s_jfree(val); return false; }
		obj->keys = nk;
	}
	obj->keys[obj->count] = s_jstrdup(key);
	obj->items[obj->count] = val;
	obj->count++;
	return true;
}

static void s_jdel(s_jval* obj, const char* key)
{
	size_t i;
	for (i = 0; i < obj->count; ++i) {
		if (strcmp(obj->keys[i], key) == 0) {
			s_jfree(obj->items[i]); free(obj->keys[i]);
			memmove(obj->items + i, obj->items + i + 1, (obj->count - i - 1) * sizeof(s_jval*));
			memmove(obj->keys + i, obj->keys + i + 1, (obj->count - i - 1) * sizeof(char*));
			obj->count--;
			return;
		}
	}
}

typedef struct s_jparse { const char* p; const char* end; int depth; } s_jparse;

static void s_jws(s_jparse* j) { while (j->p < j->end && (*j->p == ' ' || *j->p == '\t' || *j->p == '\n' || *j->p == '\r')) j->p++; }

static s_jval* s_jparse_value(s_jparse* j);

static bool s_jparse_string(s_jparse* j, char** out)
{
	char* buf = NULL; size_t n = 0, cap = 0;
	if (j->p >= j->end || *j->p != '"') return false;
	j->p++;
	for (;;) {
		unsigned char c;
		if (j->p >= j->end) { free(buf); return false; }
		c = (unsigned char)*j->p++;
		if (c == '"') break;
		if (n + 8 >= cap) { char* nb = (char*)realloc(buf, cap = cap ? cap * 2 : 64); if (!nb) { free(buf); return false; } buf = nb; }
		if (c == '\\') {
			if (j->p >= j->end) { free(buf); return false; }
			c = (unsigned char)*j->p++;
			switch (c) {
			case 'n': buf[n++] = '\n'; break;
			case 't': buf[n++] = '\t'; break;
			case 'r': buf[n++] = '\r'; break;
			case 'b': buf[n++] = '\b'; break;
			case 'f': buf[n++] = '\f'; break;
			case 'u': {
				unsigned cp = 0; int k;
				if (j->p + 4 > j->end) { free(buf); return false; }
				for (k = 0; k < 4; ++k) { char h = j->p[k]; cp = cp * 16 + (unsigned)(isdigit((unsigned char)h) ? h - '0' : (tolower((unsigned char)h) - 'a' + 10)); }
				j->p += 4;
				if (cp < 0x80) buf[n++] = (char)cp;
				else if (cp < 0x800) { buf[n++] = (char)(0xC0 | (cp >> 6)); buf[n++] = (char)(0x80 | (cp & 0x3F)); }
				else { buf[n++] = (char)(0xE0 | (cp >> 12)); buf[n++] = (char)(0x80 | ((cp >> 6) & 0x3F)); buf[n++] = (char)(0x80 | (cp & 0x3F)); }
				break;
			}
			default: buf[n++] = (char)c; break;
			}
		} else {
			buf[n++] = (char)c;
		}
	}
	if (!buf) buf = (char*)malloc(1);
	if (!buf) return false;
	buf[n] = 0;
	*out = buf;
	return true;
}

static s_jval* s_jparse_value(s_jparse* j)
{
	s_jval* v;
	s_jws(j);
	if (j->p >= j->end || j->depth > 64) return NULL;
	if (*j->p == '{') {
		v = s_jnew(S_JOBJ);
		if (!v) return NULL;
		j->p++; j->depth++;
		s_jws(j);
		if (j->p < j->end && *j->p == '}') { j->p++; j->depth--; return v; }
		for (;;) {
			char* key; s_jval* item;
			s_jws(j);
			if (!s_jparse_string(j, &key)) { s_jfree(v); return NULL; }
			s_jws(j);
			if (j->p >= j->end || *j->p != ':') { free(key); s_jfree(v); return NULL; }
			j->p++;
			item = s_jparse_value(j);
			if (!item || !s_jset(v, key, item)) { free(key); s_jfree(item); s_jfree(v); return NULL; }
			free(key);
			s_jws(j);
			if (j->p < j->end && *j->p == ',') { j->p++; continue; }
			if (j->p < j->end && *j->p == '}') { j->p++; break; }
			s_jfree(v); return NULL;
		}
		j->depth--;
		return v;
	}
	if (*j->p == '[') {
		v = s_jnew(S_JARR);
		if (!v) return NULL;
		j->p++; j->depth++;
		s_jws(j);
		if (j->p < j->end && *j->p == ']') { j->p++; j->depth--; return v; }
		for (;;) {
			s_jval* item = s_jparse_value(j);
			if (!item || !s_jpush(v, item)) { s_jfree(item); s_jfree(v); return NULL; }
			s_jws(j);
			if (j->p < j->end && *j->p == ',') { j->p++; continue; }
			if (j->p < j->end && *j->p == ']') { j->p++; break; }
			s_jfree(v); return NULL;
		}
		j->depth--;
		return v;
	}
	if (*j->p == '"') {
		char* s;
		if (!s_jparse_string(j, &s)) return NULL;
		v = s_jnew(S_JSTR);
		if (!v) { free(s); return NULL; }
		v->text = s;
		return v;
	}
	{
		const char* start = j->p;
		while (j->p < j->end && (isalnum((unsigned char)*j->p) || *j->p == '-' || *j->p == '+' || *j->p == '.')) j->p++;
		if (j->p == start) return NULL;
		v = s_jnew(S_JNUM);
		if (!v) return NULL;
		v->text = (char*)malloc((size_t)(j->p - start) + 1);
		if (!v->text) { s_jfree(v); return NULL; }
		memcpy(v->text, start, (size_t)(j->p - start));
		v->text[j->p - start] = 0;
		if (strcmp(v->text, "true") == 0 || strcmp(v->text, "false") == 0) v->type = S_JBOOL;
		else if (strcmp(v->text, "null") == 0) v->type = S_JNULL;
		return v;
	}
}

static s_jval* s_jparse_text(const char* text, size_t len)
{
	s_jparse j;
	s_jval* v;
	j.p = text; j.end = text + len; j.depth = 0;
	v = s_jparse_value(&j);
	if (!v) return NULL;
	s_jws(&j);
	if (j.p != j.end) { s_jfree(v); return NULL; }
	return v;
}

typedef struct s_jout { char* buf; size_t len, cap; bool failed; } s_jout;

static void s_jout_put(s_jout* o, const char* s, size_t n)
{
	if (o->failed) return;
	if (o->len + n + 1 > o->cap) {
		size_t ncap = o->cap ? o->cap : 4096;
		char* nb;
		while (ncap < o->len + n + 1) ncap *= 2;
		nb = (char*)realloc(o->buf, ncap);
		if (!nb) { o->failed = true; return; }
		o->buf = nb; o->cap = ncap;
	}
	memcpy(o->buf + o->len, s, n);
	o->len += n;
	o->buf[o->len] = 0;
}

static void s_jout_str(s_jout* o, const char* s)
{
	s_jout_put(o, "\"", 1);
	for (; *s; ++s) {
		unsigned char c = (unsigned char)*s;
		char esc[8];
		if (c == '"') s_jout_put(o, "\\\"", 2);
		else if (c == '\\') s_jout_put(o, "\\\\", 2);
		else if (c == '\n') s_jout_put(o, "\\n", 2);
		else if (c == '\t') s_jout_put(o, "\\t", 2);
		else if (c == '\r') s_jout_put(o, "\\r", 2);
		else if (c < 0x20) { snprintf(esc, sizeof(esc), "\\u%04x", c); s_jout_put(o, esc, 6); }
		else s_jout_put(o, (const char*)&c, 1);
	}
	s_jout_put(o, "\"", 1);
}

static void s_jout_indent(s_jout* o, int depth) { int i; for (i = 0; i < depth; ++i) s_jout_put(o, "  ", 2); }

static void s_jout_value(s_jout* o, const s_jval* v, int depth)
{
	size_t i;
	switch (v->type) {
	case S_JSTR: s_jout_str(o, v->text); break;
	case S_JNUM: case S_JBOOL: s_jout_put(o, v->text, strlen(v->text)); break;
	case S_JNULL: s_jout_put(o, "null", 4); break;
	case S_JARR:
		if (v->count == 0) { s_jout_put(o, "[]", 2); break; }
		s_jout_put(o, "[\n", 2);
		for (i = 0; i < v->count; ++i) {
			s_jout_indent(o, depth + 1);
			s_jout_value(o, v->items[i], depth + 1);
			s_jout_put(o, i + 1 < v->count ? ",\n" : "\n", i + 1 < v->count ? 2 : 1);
		}
		s_jout_indent(o, depth); s_jout_put(o, "]", 1);
		break;
	case S_JOBJ:
		if (v->count == 0) { s_jout_put(o, "{}", 2); break; }
		s_jout_put(o, "{\n", 2);
		for (i = 0; i < v->count; ++i) {
			s_jout_indent(o, depth + 1);
			s_jout_str(o, v->keys[i]);
			s_jout_put(o, ": ", 2);
			s_jout_value(o, v->items[i], depth + 1);
			s_jout_put(o, i + 1 < v->count ? ",\n" : "\n", i + 1 < v->count ? 2 : 1);
		}
		s_jout_indent(o, depth); s_jout_put(o, "}", 1);
		break;
	default: break;
	}
}

static s_jval* s_jread_file(const char* path)
{
	size_t len;
	unsigned char* text = s_read_file(path, &len);
	s_jval* v;
	if (!text) return NULL;
	v = s_jparse_text((const char*)text, len);
	free(text);
	return v;
}

static bool s_jwrite_file(const char* path, const s_jval* v)
{
	s_jout o;
	bool ok;
	memset(&o, 0, sizeof(o));
	s_jout_value(&o, v, 0);
	s_jout_put(&o, "\n", 1);
	ok = !o.failed && s_write_file(path, o.buf, o.len);
	free(o.buf);
	return ok;
}

//--------------------------------------------------------------------------------------------------
// Resolution: a table per module from wherever one lies, then names and lines onto the frames.

typedef struct s_mod_table { sym_table table; unsigned char* file; bool have; const char* how; } s_mod_table;

static bool s_table_matches(const sym_table* t, const unsigned char* id, int id_len)
{
	return id_len > 0 && t->build_id_len == id_len && memcmp(t->build_id, id, (size_t)id_len) == 0;
}

static bool s_try_table_file(const char* path, const unsigned char* id, int id_len, s_mod_table* out)
{
	sym_table t;
	if (!sym_read_file(path, &t)) return false;
	if (!s_table_matches(&t, id, id_len)) { sym_free(&t); return false; }
	out->table = t; out->have = true; out->how = "file";
	return true;
}

static void s_find_table(const s_jval* module, const char* sym_dir, s_mod_table* out)
{
	const char* name = s_jstring(module, "name");
	const char* path = s_jstring(module, "path");
	const char* id_hex = s_jstring(module, "build_id");
	unsigned char id[20];
	int id_len;
	char p[2048], dir[1024], idh[64];
	memset(out, 0, sizeof(*out));
	out->how = "none";
	id_len = id_hex ? s_unhex(id_hex, id, 20) : -1;
	if (id_len <= 0) return;
	if (!name) name = path ? s_basename(path) : "";
	// 1. The module's own slot.
	if (path && path[0]) {
		size_t len;
		unsigned char* file = s_read_file(path, &len);
		if (file) {
			sym_table t;
			if (s_slot_table(file, len, &t) && s_table_matches(&t, id, id_len)) { out->table = t; out->file = file; out->have = true; out->how = "embedded"; return; }
			free(file);
		}
	}
	// 2. <name>.sym beside the module, or in sym_dir.
	if (path && path[0]) {
		snprintf(p, sizeof(p), "%s.sym", path);
		if (s_try_table_file(p, id, id_len, out)) return;
	}
	if (sym_dir && sym_dir[0]) {
		snprintf(p, sizeof(p), "%s/%s.sym", sym_dir, name);
		if (s_try_table_file(p, id, id_len, out)) return;
		s_hex(id, id_len, idh);
		snprintf(p, sizeof(p), "%s/%s.sym", sym_dir, idh);
		if (s_try_table_file(p, id, id_len, out)) return;
		snprintf(p, sizeof(p), "%s/%s/%s.sym", sym_dir, idh, name);
		if (s_try_table_file(p, id, id_len, out)) return;
	} else if (path && path[0]) {
		s_dirname(path, dir, sizeof(dir));
		snprintf(p, sizeof(p), "%s/%s.sym", dir, name);
		if (s_try_table_file(p, id, id_len, out)) return;
	}
}

static void s_free_mod_table(s_mod_table* m)
{
	if (m->have) sym_free(&m->table);
	free(m->file);
	memset(m, 0, sizeof(*m));
}

static s_jval* s_frame_obj(const char* function, const char* file, uint32_t line)
{
	s_jval* f = s_jnew(S_JOBJ);
	if (!f) return NULL;
	s_jset(f, "function", s_jstr(function));
	if (file && file[0]) s_jset(f, "file", s_jstr(file));
	if (line) s_jset(f, "line", s_jnum(line));
	return f;
}

// Resolves one frame object in place. `top` is the first frame of its stack.
static void s_resolve_frame(s_jval* frame, const s_mod_table* tables, size_t table_count, bool top)
{
	long long mi = s_jint(frame, "module", -1);
	long long off = s_jint(frame, "offset", -1);
	sym_frame out, inl[32];
	int n = 0, k;
	uint64_t addr;
	s_jdel(frame, "function"); s_jdel(frame, "file"); s_jdel(frame, "line"); s_jdel(frame, "inlined");
	if (mi < 0 || (size_t)mi >= table_count || off < 0 || !tables[mi].have) return;
	addr = (uint64_t)off;
	if (!top && addr > 0) addr -= 1;
	if (!sym_lookup(&tables[mi].table, addr, &out, inl, 32, &n)) return;
	if (n > 0) {
		s_jval* arr = s_jnew(S_JARR);
		if (out.function[0]) s_jset(frame, "function", s_jstr(out.function));
		if (inl[n - 1].file[0]) s_jset(frame, "file", s_jstr(inl[n - 1].file));
		if (inl[n - 1].line) s_jset(frame, "line", s_jnum(inl[n - 1].line));
		for (k = 0; k < n && arr; ++k) {
			const char* file = k == 0 ? out.file : inl[k - 1].file;
			uint32_t line = k == 0 ? out.line : inl[k - 1].line;
			s_jpush(arr, s_frame_obj(inl[k].function, file, line));
		}
		if (arr) s_jset(frame, "inlined", arr);
	} else {
		if (out.function[0]) s_jset(frame, "function", s_jstr(out.function));
		if (out.file[0]) s_jset(frame, "file", s_jstr(out.file));
		if (out.line) s_jset(frame, "line", s_jnum(out.line));
	}
}

static void s_resolve_stack(s_jval* stack, const s_mod_table* tables, size_t table_count)
{
	size_t i;
	if (!stack || stack->type != S_JARR) return;
	for (i = 0; i < stack->count; ++i) if (stack->items[i]->type == S_JOBJ) s_resolve_frame(stack->items[i], tables, table_count, i == 0);
}

bool sym_resolve(const char* report_path, const char* sym_dir, sym_log_fn log, void* udata)
{
	s_jval* root = s_jread_file(report_path);
	s_jval* modules; s_jval* stack; s_jval* threads; s_jval* fault; s_jval* sig;
	s_mod_table* tables = NULL;
	size_t i, n = 0;
	bool ok;
	if (!root || root->type != S_JOBJ) { sym_logf(log, udata, "cute_sym: cannot read %s as a report", report_path); s_jfree(root); return false; }
	modules = s_jget(root, "modules");
	if (modules && modules->type == S_JARR) {
		n = modules->count;
		tables = (s_mod_table*)calloc(n ? n : 1, sizeof(s_mod_table));
		if (!tables) { s_jfree(root); return false; }
		for (i = 0; i < n; ++i) {
			if (modules->items[i]->type != S_JOBJ) continue;
			s_find_table(modules->items[i], sym_dir, &tables[i]);
			s_jset(modules->items[i], "symbols", s_jstr(tables[i].how));
		}
	}
	stack = s_jget(root, "stack");
	s_resolve_stack(stack, tables, n);
	threads = s_jget(root, "threads");
	if (threads && threads->type == S_JARR) for (i = 0; i < threads->count; ++i) s_resolve_stack(s_jget(threads->items[i], "stack"), tables, n);
	// The symbolic signature: the fault plus the top 8 non-system frames' names.
	fault = s_jget(root, "fault");
	sig = s_jget(root, "signature");
	if (stack && stack->type == S_JARR) {
		s_sha1 h;
		int named = 0, taken = 0;
		const char* kind = fault ? (s_jstring(fault, "exception") ? s_jstring(fault, "exception") : s_jstring(fault, "signal")) : NULL;
		s_sha1_init(&h);
		if (kind) s_sha1_update(&h, kind, strlen(kind));
		for (i = 0; i < stack->count && taken < 8; ++i) {
			const s_jval* f = stack->items[i];
			long long mi = s_jint(f, "module", -1);
			const s_jval* m = mi >= 0 && modules && (size_t)mi < modules->count ? modules->items[mi] : NULL;
			const s_jval* sys = m ? s_jget(m, "system") : NULL;
			const char* fn = s_jstring(f, "function");
			if (sys && sys->type == S_JBOOL && strcmp(sys->text, "true") == 0) continue;
			taken++;
			if (fn) { s_sha1_update(&h, fn, strlen(fn)); s_sha1_update(&h, "|", 1); named++; }
		}
		if (named > 0) {
			unsigned char d[20]; char hex[48];
			s_sha1_final(&h, d);
			s_hex(d, 20, hex);
			if (!sig || sig->type != S_JOBJ) { sig = s_jnew(S_JOBJ); s_jset(root, "signature", sig); }
			if (sig) s_jset(sig, "symbolic", s_jstr(hex));
		}
	}
	ok = s_jwrite_file(report_path, root);
	if (!ok) sym_logf(log, udata, "cute_sym: cannot write %s", report_path);
	for (i = 0; i < n; ++i) s_free_mod_table(&tables[i]);
	free(tables);
	s_jfree(root);
	return ok;
}

//--------------------------------------------------------------------------------------------------
// The human rendering.

static const char* s_jstr_or(const s_jval* obj, const char* key, const char* dflt)
{
	const char* s = s_jstring(obj, key);
	return s ? s : dflt;
}

static void s_print_frame(FILE* out, const s_jval* f, const s_jval* modules, size_t index)
{
	long long mi = s_jint(f, "module", -1);
	const char* mod = mi >= 0 && modules && (size_t)mi < modules->count ? s_jstr_or(modules->items[mi], "name", "?") : "?";
	const char* fn = s_jstring(f, "function");
	const char* file = s_jstring(f, "file");
	long long line = s_jint(f, "line", 0);
	const s_jval* inl = s_jget(f, "inlined");
	char loc[64];
	snprintf(loc, sizeof(loc), "%s+0x%llx", mod, s_jint(f, "offset", 0));
	fprintf(out, "  %3u  %-28s", (unsigned)index, loc);
	if (fn) fprintf(out, "  %s", fn);
	if (file) fprintf(out, "  %s", file);
	if (line) fprintf(out, ":%lld", line);
	fprintf(out, "\n");
	if (inl && inl->type == S_JARR) {
		size_t k;
		for (k = 0; k < inl->count; ++k) {
			const s_jval* g = inl->items[k];
			fprintf(out, "          inlined  %s", s_jstr_or(g, "function", "?"));
			if (s_jstring(g, "file")) fprintf(out, "  %s", s_jstring(g, "file"));
			if (s_jint(g, "line", 0)) fprintf(out, ":%lld", s_jint(g, "line", 0));
			fprintf(out, "\n");
		}
	}
}

static void s_print_stack(FILE* out, const s_jval* stack, const s_jval* modules)
{
	size_t i;
	if (!stack || stack->type != S_JARR) { fprintf(out, "  (none)\n"); return; }
	for (i = 0; i < stack->count; ++i) s_print_frame(out, stack->items[i], modules, i);
}

bool sym_print(const char* report_path, FILE* out)
{
	s_jval* root = s_jread_file(report_path);
	const s_jval* machine; const s_jval* fault; const s_jval* modules; const s_jval* state; const s_jval* crumbs; const s_jval* att; const s_jval* threads;
	size_t i;
	if (!root || root->type != S_JOBJ) { s_jfree(root); return false; }
	machine = s_jget(root, "machine"); fault = s_jget(root, "fault"); modules = s_jget(root, "modules");
	state = s_jget(root, "state"); crumbs = s_jget(root, "breadcrumbs"); att = s_jget(root, "attachments"); threads = s_jget(root, "threads");
	fprintf(out, "cute_crash report  %s\n", s_jstr_or(root, "id", "?"));
	fprintf(out, "app: %s %s", s_jstr_or(root, "app", "?"), s_jstr_or(root, "version", "?"));
	if (s_jstring(root, "build")) fprintf(out, " (%s%s%s)", s_jstring(root, "build"), s_jstring(root, "config") ? ", " : "", s_jstr_or(root, "config", ""));
	fprintf(out, "\nkind: %s   time: %s", s_jstr_or(root, "kind", "?"), s_jstr_or(root, "time", "?"));
	if (s_jget(root, "uptime")) fprintf(out, "   uptime: %s s", s_jget(root, "uptime")->text ? s_jget(root, "uptime")->text : "?");
	fprintf(out, "\n");
	if (machine) {
		fprintf(out, "os: %s   arch: %s\n", s_jstr_or(machine, "os", "?"), s_jstr_or(machine, "arch", "?"));
		fprintf(out, "cpu: %s (%lld threads), %lld MB", s_jstr_or(machine, "cpu", "?"), s_jint(machine, "threads", 0), s_jint(machine, "ram_mb", 0));
		if (s_jstring(machine, "gpu")) fprintf(out, "   gpu: %s %s %s", s_jstring(machine, "gpu"), s_jstr_or(machine, "gpu_driver", ""), s_jstr_or(machine, "backend", ""));
		fprintf(out, "\n");
	}
	if (fault) {
		if (s_jstring(fault, "exception")) fprintf(out, "fault: %s (%s), %s at %s", s_jstring(fault, "exception"), s_jstr_or(fault, "code", "?"), s_jstr_or(fault, "access", "?"), s_jstr_or(fault, "address", "?"));
		else if (s_jstring(fault, "signal")) fprintf(out, "fault: %s (%s) at %s", s_jstring(fault, "signal"), s_jstr_or(fault, "code", "?"), s_jstr_or(fault, "address", "?"));
		else if (s_jget(fault, "seconds")) fprintf(out, "hang: %s s without a heartbeat", s_jget(fault, "seconds")->text);
		fprintf(out, ", thread %lld \"%s\"\n", s_jint(fault, "thread", 0), s_jstr_or(fault, "thread_name", ""));
	}
	fprintf(out, "stack:\n");
	s_print_stack(out, s_jget(root, "stack"), modules);
	if (threads && threads->type == S_JARR) {
		for (i = 0; i < threads->count; ++i) {
			fprintf(out, "thread %lld \"%s\":\n", s_jint(threads->items[i], "id", 0), s_jstr_or(threads->items[i], "name", ""));
			s_print_stack(out, s_jget(threads->items[i], "stack"), modules);
		}
	}
	if (state && state->type == S_JOBJ && state->count) {
		fprintf(out, "state:\n");
		for (i = 0; i < state->count; ++i) fprintf(out, "  %s: %s\n", state->keys[i], state->items[i]->text ? state->items[i]->text : "");
	}
	if (crumbs && crumbs->type == S_JARR && crumbs->count) {
		fprintf(out, "breadcrumbs (%u):\n", (unsigned)crumbs->count);
		for (i = 0; i < crumbs->count; ++i) {
			const s_jval* t = s_jget(crumbs->items[i], "t");
			fprintf(out, "  [%s] %s\n", t && t->text ? t->text : "?", s_jstr_or(crumbs->items[i], "msg", ""));
		}
	}
	if (modules && modules->type == S_JARR) {
		fprintf(out, "modules:\n");
		for (i = 0; i < modules->count; ++i) {
			const s_jval* m = modules->items[i];
			fprintf(out, "  %-24s %-18s %10lld  %s  %s\n", s_jstr_or(m, "name", "?"), s_jstr_or(m, "base", "?"), s_jint(m, "size", 0), s_jstr_or(m, "build_id", "-"), s_jstr_or(m, "symbols", ""));
		}
	}
	if (att && att->type == S_JARR && att->count) {
		fprintf(out, "attachments:\n");
		for (i = 0; i < att->count; ++i) fprintf(out, "  %s (%s, %lld bytes)\n", s_jstr_or(att->items[i], "name", "?"), s_jstr_or(att->items[i], "type", "?"), s_jint(att->items[i], "bytes", 0));
	}
	s_jfree(root);
	return true;
}

//--------------------------------------------------------------------------------------------------
// The command line tool.

static void s_cli_log(void* udata, const char* msg) { (void)udata; fprintf(stderr, "%s\n", msg); }

static void s_dump_table(const sym_table* t, FILE* out)
{
	char id[48];
	uint32_t i, li = 0, ii = 0;
	s_hex(t->build_id, t->build_id_len, id);
	fprintf(out, "CUTESYM %d  arch %d  build_id %s\n%u functions, %u line rows, %u inline sites, %u files, %u string bytes\n",
		SYM_VERSION, t->arch, id, (unsigned)t->func_count, (unsigned)t->line_count, (unsigned)t->inline_count, (unsigned)t->file_count, (unsigned)t->strings_len);
	for (i = 0; i < t->func_count; ++i) {
		const unsigned char* f = (const unsigned char*)t->funcs + (size_t)i * 16;
		uint64_t start = s_rd64(f), endv = start + s_rd32(f + 8);
		fprintf(out, "0x%llx 0x%x  %s\n", (unsigned long long)start, (unsigned)s_rd32(f + 8), s_str(t, s_rd32(f + 12)));
		while (li < t->line_count && s_rd64((const unsigned char*)t->lines + (size_t)li * 24) < start) ++li;
		for (; li < t->line_count; ++li) {
			const unsigned char* l = (const unsigned char*)t->lines + (size_t)li * 24;
			if (s_rd64(l) >= endv) break;
			fprintf(out, "  0x%llx +0x%x  %s:%u\n", (unsigned long long)s_rd64(l), (unsigned)s_rd32(l + 8), s_str(t, s_rd32(l + 12)), (unsigned)s_rd32(l + 16));
		}
		while (ii < t->inline_count && s_rd64((const unsigned char*)t->inlines + (size_t)ii * 32) < start) ++ii;
		for (; ii < t->inline_count; ++ii) {
			const unsigned char* n = (const unsigned char*)t->inlines + (size_t)ii * 32;
			if (s_rd64(n) >= endv) break;
			fprintf(out, "  inline 0x%llx +0x%x  %s <- %s:%u  parent %d\n", (unsigned long long)s_rd64(n), (unsigned)s_rd32(n + 8), s_str(t, s_rd32(n + 12)), s_str(t, s_rd32(n + 16)), (unsigned)s_rd32(n + 20), (int)s_rd32(n + 24));
		}
	}
}

int sym_main(int argc, char** argv)
{
	const char* cmd = argc > 1 ? argv[1] : NULL;
	if (!cmd || strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
		fprintf(stderr,
			"cute_sym <binary> [--debug <path>] [-o <out.sym>] [--embed] [--optional]   build a table (default: <binary>.sym beside it; --optional: no debug info is not an error)\n"
			"cute_sym resolve <report.json> [--symbols <dir>]              symbolicate a report in place\n"
			"cute_sym print <report.json>                                  render a report\n"
			"cute_sym dump <table.sym | binary with a slot>                list a table\n");
		return cmd ? 0 : 1;
	}
	if (strcmp(cmd, "resolve") == 0) {
		const char* dir = NULL;
		int i;
		if (argc < 3) { fprintf(stderr, "cute_sym resolve: a report path is required\n"); return 1; }
		for (i = 3; i + 1 < argc; ++i) if (strcmp(argv[i], "--symbols") == 0) dir = argv[++i];
		return sym_resolve(argv[2], dir, s_cli_log, NULL) ? 0 : 1;
	}
	if (strcmp(cmd, "print") == 0) {
		if (argc < 3) { fprintf(stderr, "cute_sym print: a report path is required\n"); return 1; }
		return sym_print(argv[2], stdout) ? 0 : 1;
	}
	if (strcmp(cmd, "dump") == 0) {
		sym_table t;
		size_t len;
		unsigned char* file;
		if (argc < 3) { fprintf(stderr, "cute_sym dump: a path is required\n"); return 1; }
		file = s_read_file(argv[2], &len);
		if (!file) { fprintf(stderr, "cute_sym: cannot read %s\n", argv[2]); return 1; }
		if (!sym_read(file, len, &t) && !s_slot_table(file, len, &t)) { fprintf(stderr, "cute_sym: %s holds no table\n", argv[2]); free(file); return 1; }
		s_dump_table(&t, stdout);
		free(file);
		return 0;
	}
	{
		const char* binary = cmd;
		const char* debug = NULL;
		const char* out = NULL;
		bool embed = false, optional = false;
		char path[2048];
		sym_table t;
		int i, rc = 0;
		for (i = 2; i < argc; ++i) {
			if (strcmp(argv[i], "--debug") == 0 && i + 1 < argc) debug = argv[++i];
			else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) out = argv[++i];
			else if (strcmp(argv[i], "--embed") == 0) embed = true;
			else if (strcmp(argv[i], "--optional") == 0) optional = true; // No debug info is not a failure: a build step on a library that may lack it.
			else { fprintf(stderr, "cute_sym: unknown argument %s\n", argv[i]); return 1; }
		}
		if (!sym_build(binary, debug, &t, s_cli_log, NULL)) return optional ? 0 : 1;
		if (!out && !embed) { snprintf(path, sizeof(path), "%s.sym", binary); out = path; }
		if (out && !sym_write(&t, out)) { fprintf(stderr, "cute_sym: cannot write %s\n", out); rc = 1; }
		else if (out) fprintf(stderr, "cute_sym: wrote %s (%u functions, %u line rows, %u inline sites, %u bytes)\n", out, (unsigned)t.func_count, (unsigned)t.line_count, (unsigned)t.inline_count, (unsigned)t.len);
		if (embed && !sym_embed(&t, binary, s_cli_log, NULL)) rc = 1;
		else if (embed) fprintf(stderr, "cute_sym: embedded %u bytes into %s\n", (unsigned)t.len, binary);
		sym_free(&t);
		return rc;
	}
}

#ifdef CUTE_SYM_MAIN
int main(int argc, char** argv) { return sym_main(argc, argv); }
#endif

#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // CUTE_SYM_IMPLEMENTATION_ONCE
#endif // CUTE_SYM_IMPLEMENTATION

/*
	------------------------------------------------------------------------------
	This software is available under 2 licenses - you may choose the one you like.
	------------------------------------------------------------------------------
	ALTERNATIVE A - zlib license
	Copyright (c) 2026 bullno1, Randy Gaul https://randygaul.github.io/
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
