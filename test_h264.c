// Conformance harness for cute_h264.h. Encodes a synthetic pattern, then the caller decodes it
// with ffmpeg and compares against the raw dump this also writes. I_PCM is lossless, so the
// comparison is EXACT -- any difference at all is a bug, which is exactly the property worth
// having while the bitstream layer is being built.
#define CUTE_H264_IMPLEMENTATION
#include "libraries/cute/cute_h264.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

int main(int argc, char** argv)
{
	int w = argc > 1 ? atoi(argv[1]) : 320;
	int h = argc > 2 ? atoi(argv[2]) : 176;
	int frames = argc > 3 ? atoi(argv[3]) : 6;
	int qp = argc > 4 ? atoi(argv[4]) : 26;
	int flat = argc > 5 ? atoi(argv[5]) : 0;

	unsigned char* rgba = (unsigned char*)malloc((size_t)w * h * 4);
	ch_encoder_t* e = ch_encoder_make(w, h, 30);
	if (e) ch_encoder_qp(e, qp);
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
				if (flat == 1) { p[0] = 90; p[1] = 140; p[2] = 60; p[3] = 255; continue; }
				if (flat == 2) {
					// A smooth field translated by a NON-integer amount each frame. The pattern
					// above shifts by whole pixels, which a broken sub-sample interpolator would
					// survive untouched because the motion search would never ask it for one.
					double sx = x - f * 1.4, sy = y - f * 0.7;
					int v = (int)(127.5 + 100.0 * sin(sx * 0.11) * cos(sy * 0.09));
					p[0] = (unsigned char)v;
					p[1] = (unsigned char)(255 - v);
					p[2] = (unsigned char)((v * 3) & 255);
					p[3] = 255;
					continue;
				}
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
		// For I_PCM the source planes ARE the reconstruction. For a compressed frame the decoder must
		// reproduce the encoder's own reconstruction exactly -- encoder and decoder run the same
		// inverse transform and the same intra prediction, so any difference at all is a bug in one
		// of them, and this is what makes the round trip a real conformance test rather than a PSNR
		// eyeball.
		const uint8_t* py = e->pic.ref_y[0];
		const uint8_t* pb = e->pic.ref_cb[0];
		const uint8_t* pr = e->pic.ref_cr[0];
		for (int y = 0; y < h; ++y) fwrite(py + (size_t)y * e->luma_stride, 1, (size_t)w, yuv);
		for (int y = 0; y < h / 2; ++y) fwrite(pb + (size_t)y * e->chroma_stride, 1, (size_t)(w / 2), yuv);
		for (int y = 0; y < h / 2; ++y) fwrite(pr + (size_t)y * e->chroma_stride, 1, (size_t)(w / 2), yuv);
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
