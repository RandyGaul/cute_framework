// Conformance harness for cute_h264.h. Encodes a synthetic pattern, then the caller decodes it
// with ffmpeg and compares against the raw dump this also writes. I_PCM is lossless, so the
// comparison is EXACT -- any difference at all is a bug, which is exactly the property worth
// having while the bitstream layer is being built.
#define CUTE_H264_IMPLEMENTATION
#include "libraries/cute/cute_h264.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char** argv)
{
	int w = argc > 1 ? atoi(argv[1]) : 320;
	int h = argc > 2 ? atoi(argv[2]) : 176;
	int frames = argc > 3 ? atoi(argv[3]) : 6;

	unsigned char* rgba = (unsigned char*)malloc((size_t)w * h * 4);
	ch_encoder_t* e = ch_encoder_make(w, h, 30);
	if (!e) { printf("make failed: %s\n", ch_error_reason); return 1; }

	FILE* raw = fopen("ref.rgba", "wb");
	FILE* yuv = fopen("ref.yuv", "wb");
	for (int f = 0; f < frames; ++f) {
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				unsigned char* p = rgba + ((size_t)y * w + x) * 4;
				// Hard edges, saturated primaries and a moving element: the combination that
				// catches colour-matrix, stride and chroma-siting mistakes. A smooth gradient
				// would hide all three.
				int cx = (x * 8 / w), cy = (y * 8 / h);
				int checker = (cx + cy) & 1;
				int bar = (x + f * 7) % w < w / 8;
				p[0] = (unsigned char)(checker ? 255 : (x * 255 / (w - 1)));
				p[1] = (unsigned char)(bar ? 255 : (y * 255 / (h - 1)));
				p[2] = (unsigned char)(checker ? (f * 37) & 255 : 24);
				p[3] = 255;
			}
		}
		fwrite(rgba, 1, (size_t)w * h * 4, raw);
		if (!ch_encoder_frame(e, rgba)) { printf("frame failed: %s\n", ch_error_reason); return 1; }
		// Dump the encoder's OWN yuv420p, cropped, in ffmpeg's plane order. I_PCM is lossless in
		// YUV but not in RGB -- 4:2:0 throws chroma away before the codec ever sees it -- so this
		// is the buffer a conformant decoder has to reproduce exactly.
		for (int y = 0; y < h; ++y) fwrite(e->y + (size_t)y * e->luma_stride, 1, (size_t)w, yuv);
		for (int y = 0; y < h / 2; ++y) fwrite(e->cb + (size_t)y * e->chroma_stride, 1, (size_t)(w / 2), yuv);
		for (int y = 0; y < h / 2; ++y) fwrite(e->cr + (size_t)y * e->chroma_stride, 1, (size_t)(w / 2), yuv);
	}
	fclose(raw);
	fclose(yuv);

	if (!ch_encoder_save(e, "out.264")) { printf("save failed: %s\n", ch_error_reason); return 1; }
	int size = 0;
	ch_encoder_data(e, &size);
	printf("wrote out.264: %d bytes, %d frames, %dx%d (%.2f bytes/pixel)\n",
		size, frames, w, h, (double)size / ((double)w * h * frames));
	ch_encoder_destroy(e);
	free(rgba);
	return 0;
}
