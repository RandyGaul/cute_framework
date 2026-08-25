// Round trip: an .mp4 written by the encoder, read back, and decoded -- the pictures must match
// what the same stream produces in its raw Annex-B framing.
#define CUTE_H264_IMPLEMENTATION
#include "cute/cute_h264.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char* decode_all(const void* data, int size, int* out_n, int* w, int* h)
{
	ch_decoder_t* d = ch_decoder_make(data, size);
	if (!d) { printf("make: %s\n", ch_decoder_error); return NULL; }
	unsigned char* all = NULL;
	int n = 0, cap = 0;
	while (ch_decoder_next(d)) {
		ch_decoder_size(d, w, h);
		int ls = 0, cs = 0;
		const void *cb = NULL, *cr = NULL;
		const unsigned char* y = (const unsigned char*)ch_decoder_yuv(d, &ls, &cs, &cb, &cr);
		int need = *w * *h * 3 / 2;
		if (n + need > cap) { cap = (n + need) * 2; all = (unsigned char*)realloc(all, (size_t)cap); }
		for (int i = 0; i < *h; ++i) memcpy(all + n + i * *w, y + (size_t)i * ls, (size_t)*w);
		n += *w * *h;
		for (int c = 0; c < 2; ++c) {
			const unsigned char* p = (const unsigned char*)(c ? cr : cb);
			for (int i = 0; i < *h / 2; ++i) memcpy(all + n + i * (*w / 2), p + (size_t)i * cs, (size_t)(*w / 2));
			n += (*w / 2) * (*h / 2);
		}
	}
	if (ch_decoder_error) { printf("decode: %s\n", ch_decoder_error); free(all); ch_decoder_destroy(d); return NULL; }
	printf("  fps from the stream: %d\n", ch_decoder_fps(d));
	ch_decoder_destroy(d);
	*out_n = n;
	return all;
}

static void* slurp(const char* path, int* size)
{
	FILE* f = fopen(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
	void* p = malloc((size_t)n);
	if (fread(p, 1, (size_t)n, f) != (size_t)n) { free(p); fclose(f); return NULL; }
	fclose(f);
	*size = (int)n;
	return p;
}

int main(int argc, char** argv)
{
	const char* a264 = argc > 1 ? argv[1] : "out.264";
	const char* amp4 = argc > 2 ? argv[2] : "out.mp4";
	int n264 = 0, nmp4 = 0;
	void* raw = slurp(a264, &n264);
	void* mp4 = slurp(amp4, &nmp4);
	if (!raw || !mp4) { printf("cannot read inputs\n"); return 1; }

	int un = 0;
	const void* back = ch_mp4_unwrap(mp4, nmp4, &un);
	if (!back) { printf("unwrap failed: %s\n", ch_error_reason); return 1; }

	int na = 0, nb = 0, w = 0, h = 0, w2 = 0, h2 = 0;
	unsigned char* pa = decode_all(raw, n264, &na, &w, &h);
	unsigned char* pb = decode_all(back, un, &nb, &w2, &h2);
	if (!pa || !pb) return 1;
	if (na != nb || w != w2 || h != h2 || memcmp(pa, pb, (size_t)na) != 0) {
		printf("MISMATCH: %d vs %d bytes, %dx%d vs %dx%d\n", na, nb, w, h, w2, h2);
		return 1;
	}
	printf("mp4 round trip identical: %d frames of %dx%d\n", na / (w * h * 3 / 2), w, h);
	return 0;
}
