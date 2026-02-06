/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Parses SDL3 private headers at build time to extract struct layout offsets
// needed to reach ID3D12Device* from SDL_GPUDevice* on Windows.
// No CF runtime or SDL required -- only standard C.
//
// Usage: gen_d3d12_layout <SDL_sysgpu.h> <SDL_gpu_d3d12.c> <output.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 2048

// -------------------------------------------------------------------------------------------------
// Helpers

static void panic(const char* msg)
{
	fprintf(stderr, "gen_d3d12_layout ERROR: %s\n", msg);
	exit(1);
}

static char* read_file(const char* path)
{
	FILE* f = fopen(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* buf = (char*)malloc(size + 1);
	if (!buf) { fclose(f); return NULL; }
	fread(buf, 1, size, f);
	buf[size] = 0;
	fclose(f);
	return buf;
}

static const char* skip_ws(const char* s)
{
	while (*s == ' ' || *s == '\t') s++;
	return s;
}

static int str_contains(const char* hay, const char* needle)
{
	return strstr(hay, needle) != NULL;
}

// Read one line into buf (strips trailing \r), returns pointer past \n.
static const char* next_line(const char* p, char* buf, int bufsize)
{
	int i = 0;
	while (*p && *p != '\n' && i < bufsize - 1) buf[i++] = *p++;
	buf[i] = 0;
	if (i > 0 && buf[i - 1] == '\r') buf[--i] = 0;
	if (*p == '\n') p++;
	return p;
}

// Find the opening brace of a struct definition (skips forward declarations).
// Returns pointer just past the '{'.
static const char* find_struct_body(const char* content, const char* name)
{
	char pattern[256];
	snprintf(pattern, sizeof(pattern), "struct %s", name);
	const char* p = content;
	while ((p = strstr(p, pattern)) != NULL) {
		p += strlen(pattern);
		const char* q = p;
		while (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n') q++;
		if (*q == '{') return q + 1;
	}
	return NULL;
}

static int align_up(int offset, int alignment)
{
	return (offset + alignment - 1) & ~(alignment - 1);
}

// -------------------------------------------------------------------------------------------------
// Preprocessor conditional evaluator (assumes Windows desktop, non-Xbox, non-GDK)

typedef struct
{
	int stack[16];
	int depth;
} PPState;

static void pp_init(PPState* pp)
{
	pp->depth = 0;
	pp->stack[0] = 1;
}

static int pp_active(PPState* pp)
{
	return pp->stack[pp->depth];
}

static int macro_defined(const char* name)
{
	if (strstr(name, "SDL_PLATFORM_XBOXONE")) return 0;
	if (strstr(name, "SDL_PLATFORM_XBOXSERIES")) return 0;
	if (strstr(name, "HAVE_IDXGIINFOQUEUE")) return 1;
	if (strstr(name, "USE_PIX_RUNTIME")) return 1;
	fprintf(stderr, "WARNING: unknown macro '%s', assuming defined\n", name);
	return 1;
}

// Returns 1 if the line was a preprocessor directive.
static int pp_process(PPState* pp, const char* line)
{
	const char* p = skip_ws(line);
	if (*p != '#') return 0;
	p = skip_ws(p + 1);

	if (strncmp(p, "ifdef ", 6) == 0) {
		int parent = pp->stack[pp->depth];
		pp->stack[++pp->depth] = parent && macro_defined(p + 6);
		return 1;
	}
	if (strncmp(p, "ifndef ", 7) == 0) {
		int parent = pp->stack[pp->depth];
		pp->stack[++pp->depth] = parent && !macro_defined(p + 7);
		return 1;
	}
	if (strncmp(p, "if ", 3) == 0) {
		// Handle #if !(defined(X) || defined(Y)) and similar.
		const char* expr = p + 3;
		int negated = str_contains(expr, "!(");
		int any_defined = 0;
		const char* d = expr;
		while ((d = strstr(d, "defined(")) != NULL) {
			d += 8;
			char name[256] = {0};
			int i = 0;
			while (*d && *d != ')' && i < 255) name[i++] = *d++;
			name[i] = 0;
			if (macro_defined(name)) any_defined = 1;
		}
		int result = negated ? !any_defined : any_defined;
		int parent = pp->stack[pp->depth];
		pp->stack[++pp->depth] = parent && result;
		return 1;
	}
	if (strncmp(p, "else", 4) == 0 && (p[4] == 0 || p[4] == ' ' || p[4] == '\t' || p[4] == '\r' || p[4] == '/')) {
		int parent = pp->stack[pp->depth - 1];
		pp->stack[pp->depth] = parent && !pp->stack[pp->depth];
		return 1;
	}
	if (strncmp(p, "endif", 5) == 0) {
		if (pp->depth > 0) pp->depth--;
		return 1;
	}
	return 1; // Other directives (#define, #include, etc.)
}

// -------------------------------------------------------------------------------------------------
// SDL_GPUDevice: count function pointer slots before driverData

static int count_gpu_device_fn_slots(const char* sysgpu)
{
	const char* body = find_struct_body(sysgpu, "SDL_GPUDevice");
	if (!body) panic("could not find struct SDL_GPUDevice definition");

	int count = 0;
	char line[MAX_LINE];
	const char* p = body;
	while (*p) {
		p = next_line(p, line, sizeof(line));
		if (str_contains(line, "driverData;")) break;
		if (str_contains(line, "(*")) count++;
	}
	if (count == 0) panic("no function pointers found in SDL_GPUDevice before driverData");
	return count;
}

// -------------------------------------------------------------------------------------------------
// D3D12Renderer: compute byte offset of ID3D12Device *device

// Count fields in a simple struct (all assumed pointer-sized).
static int count_struct_fields(const char* content, const char* name)
{
	const char* body = find_struct_body(content, name);
	if (!body) return -1;

	int count = 0;
	char line[MAX_LINE];
	const char* p = body;
	while (*p) {
		p = next_line(p, line, sizeof(line));
		const char* t = skip_ws(line);
		if (*t == '}') break;
		if (str_contains(line, ";") && *t != '/' && *t != '#' && *t != 0) count++;
	}
	return count;
}

static int compute_d3d12_device_offset(const char* d3d12)
{
	// Parse WinPixEventRuntimeFns to learn its size.
	int pix_fields = count_struct_fields(d3d12, "WinPixEventRuntimeFns");
	int pix_size;
	if (pix_fields > 0) {
		pix_size = pix_fields * 8; // Each field is a function pointer typedef.
		printf("  WinPixEventRuntimeFns: %d fields, %d bytes\n", pix_fields, pix_size);
	} else {
		pix_size = 24;
		printf("  WinPixEventRuntimeFns: not found, assuming 24 bytes\n");
	}

	const char* body = find_struct_body(d3d12, "D3D12Renderer");
	if (!body) panic("could not find struct D3D12Renderer definition");

	PPState pp;
	pp_init(&pp);

	int offset = 0;
	char line[MAX_LINE];
	const char* p = body;

	while (*p) {
		p = next_line(p, line, sizeof(line));
		const char* t = skip_ws(line);

		if (*t == 0) continue;
		if (*t == '}') break;
		if (t[0] == '/' && t[1] == '/') continue;
		if (pp_process(&pp, line)) continue;
		if (!pp_active(&pp)) continue;
		if (!str_contains(line, ";")) continue;

		// Target field found.
		if (str_contains(line, "ID3D12Device") && str_contains(line, "device")) {
			offset = align_up(offset, 8);
			return offset;
		}

		// Classify field type.
		int size, align;
		if (str_contains(line, "WinPixEventRuntimeFns")) {
			size = pix_size;
			align = 8;
		} else if (str_contains(line, "BOOL ") || str_contains(line, "BOOL\t")) {
			size = 4;
			align = 4;
		} else if (str_contains(line, "*")) {
			size = 8;
			align = 8;
		} else {
			fprintf(stderr, "WARNING: unknown field type: '%s', assuming 8 bytes\n", t);
			size = 8;
			align = 8;
		}

		offset = align_up(offset, align);
		offset += size;
	}

	panic("ID3D12Device *device not found in D3D12Renderer");
	return -1;
}

// -------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <SDL_sysgpu.h> <SDL_gpu_d3d12.c> <output.h>\n", argv[0]);
		return 1;
	}

	char* sysgpu = read_file(argv[1]);
	if (!sysgpu) { fprintf(stderr, "ERROR: could not read %s\n", argv[1]); return 1; }

	char* d3d12 = read_file(argv[2]);
	if (!d3d12) { fprintf(stderr, "ERROR: could not read %s\n", argv[2]); return 1; }

	printf("Parsing SDL_GPUDevice from %s ...\n", argv[1]);
	int fn_count = count_gpu_device_fn_slots(sysgpu);
	printf("  function pointer slots before driverData: %d\n", fn_count);

	printf("Parsing D3D12Renderer from %s ...\n", argv[2]);
	int device_offset = compute_d3d12_device_offset(d3d12);
	printf("  ID3D12Device byte offset: %d\n", device_offset);

	FILE* out = fopen(argv[3], "w");
	if (!out) { fprintf(stderr, "ERROR: could not write %s\n", argv[3]); return 1; }

	fprintf(out, "// Auto-generated by gen_d3d12_layout -- do not edit.\n");
	fprintf(out, "// Parsed from SDL3 source to locate ID3D12Device* in private structs.\n");
	fprintf(out, "// This is done to fetch the device pointer, pretty much just to silence dumb warnings.\n");
	fprintf(out, "#define SDL_GPU_DEVICE_FN_SLOT_COUNT %d\n", fn_count);
	fprintf(out, "#define D3D12_RENDERER_DEVICE_BYTE_OFFSET %d\n", device_offset);

	fclose(out);
	free(sysgpu);
	free(d3d12);

	printf("Wrote %s\n", argv[3]);
	return 0;
}
