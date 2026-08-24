// Decodes an Annex-B file with cute_h264.h's own decoder and writes the pictures out as yuv420p,
// so they can be compared byte for byte against what the encoder reconstructed and against what
// ffmpeg decodes from the same bytes.
#define CUTE_H264_IMPLEMENTATION
#include "cute/cute_h264.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	const char* in = argc > 1 ? argv[1] : "out.264";
	const char* out = argc > 2 ? argv[2] : "ours.yuv";

	FILE* fp = fopen(in, "rb");
	if (!fp) { printf("cannot open %s\n", in); return 1; }
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char* data = (unsigned char*)malloc((size_t)size);
	if (fread(data, 1, (size_t)size, fp) != (size_t)size) { printf("short read\n"); return 1; }
	fclose(fp);

	ch_decoder_t* d = ch_decoder_make(data, (int)size);
	if (!d) { printf("make failed: %s\n", ch_decoder_error); return 1; }

	FILE* o = fopen(out, "wb");
	int frames = 0, w = 0, h = 0;
	while (ch_decoder_next(d)) {
		ch_decoder_size(d, &w, &h);
		int ls = 0, cs = 0;
		const void *cb = NULL, *cr = NULL;
		const unsigned char* y = (const unsigned char*)ch_decoder_yuv(d, &ls, &cs, &cb, &cr);
		if (!y) { printf("no picture: %s\n", ch_decoder_error); return 1; }
		for (int i = 0; i < h; ++i) fwrite(y + (size_t)i * ls, 1, (size_t)w, o);
		for (int i = 0; i < h / 2; ++i) fwrite((const unsigned char*)cb + (size_t)i * cs, 1, (size_t)(w / 2), o);
		for (int i = 0; i < h / 2; ++i) fwrite((const unsigned char*)cr + (size_t)i * cs, 1, (size_t)(w / 2), o);
		++frames;
	}
	fclose(o);
	// A decode that stops early is a failure even though pictures came out of it, so the error is
	// what gets checked rather than the frame count.
	if (ch_decoder_error) { printf("decode failed after %d frames: %s\n", frames, ch_decoder_error); return 1; }
	printf("decoded %d frames, %dx%d\n", frames, w, h);
	ch_decoder_destroy(d);
	free(data);
	return 0;
}
