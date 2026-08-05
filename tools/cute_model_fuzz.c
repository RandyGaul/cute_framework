/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Mutation fuzzer for cute_model.h. Games load models from disk, and once mods or
// user-generated content enter the picture those bytes are hostile: the loader must
// reject garbage rather than read out of bounds. This harness takes seed files, applies
// deterministic random mutations (bit flips, chunk-length corruption, truncation, JSON
// token damage), and hands the result to cm_load. Every run must end in either a clean
// model or a NULL + error string -- never a crash.
//
// Deterministic by design: the PRNG is seeded from the command line, so any failure
// reproduces exactly. Build with a sanitizer (see the build command in the header
// comment below) to catch reads that stay inside the process but outside the buffer.
//
//     cl /fsanitize=address /Zi /Od /I libraries tools\cute_model_fuzz.c
//     cute_model_fuzz.exe <seed_file> [iterations] [start_seed]
//
// Without a seed file it fuzzes a small built-in glTF, so it runs anywhere.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CKIT_IMPLEMENTATION
#include <cute/ckit.h>
#define CUTE_MODEL_IMPLEMENTATION
#include <cute/cute_model.h>

// xorshift64*: tiny, deterministic, good enough to drive mutations.
static uint64_t s_rng_state;
static uint32_t s_rand(void)
{
	uint64_t x = s_rng_state;
	x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
	s_rng_state = x;
	return (uint32_t)((x * 0x2545F4914F6CDD1DULL) >> 32);
}
static uint32_t s_rand_below(uint32_t n) { return n ? s_rand() % n : 0; }

// A self-contained glTF seed: base64 buffer, a skin, an animation, morph targets, and
// materials, so mutations reach every parsing path even with no seed file on hand.
static const char* s_builtin_seed =
"{\"asset\":{\"version\":\"2.0\"},"
"\"scenes\":[{\"nodes\":[0]}],"
"\"nodes\":[{\"name\":\"n\",\"mesh\":0,\"skin\":0,\"extras\":{\"a\":1},\"translation\":[1,2,3]},"
          "{\"name\":\"j\",\"rotation\":[0,0,0,1]}],"
"\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"TEXCOORD_0\":1,\"JOINTS_0\":3,\"WEIGHTS_0\":4},"
                             "\"indices\":2,\"material\":0,\"targets\":[{\"POSITION\":0}]}],\"weights\":[0.5]}],"
"\"skins\":[{\"joints\":[1],\"inverseBindMatrices\":5}],"
"\"animations\":[{\"name\":\"a\",\"channels\":[{\"sampler\":0,\"target\":{\"node\":1,\"path\":\"rotation\"}}],"
                 "\"samplers\":[{\"input\":6,\"output\":7,\"interpolation\":\"CUBICSPLINE\"}]}],"
"\"materials\":[{\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":0,\"texCoord\":1,"
   "\"extensions\":{\"KHR_texture_transform\":{\"offset\":[0.1,0.2],\"rotation\":0.5}}}},\"alphaMode\":\"MASK\"}],"
"\"textures\":[{\"source\":0}],"
"\"images\":[{\"uri\":\"data:image/png;base64,iVBORw0KGgo=\"}],"
"\"buffers\":[{\"uri\":\"data:application/octet-stream;base64,"
   "AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAABAAIAAAAAAAAAgD8AAAAAAAAAAA"
   "AAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAAAAAACAPw==\","
   "\"byteLength\":192}],"
"\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
                 "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":24},"
                 "{\"buffer\":0,\"byteOffset\":60,\"byteLength\":6},"
                 "{\"buffer\":0,\"byteOffset\":66,\"byteLength\":24},"
                 "{\"buffer\":0,\"byteOffset\":90,\"byteLength\":48},"
                 "{\"buffer\":0,\"byteOffset\":128,\"byteLength\":64}],"
"\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"type\":\"VEC3\",\"count\":3},"
               "{\"bufferView\":1,\"componentType\":5126,\"type\":\"VEC2\",\"count\":3},"
               "{\"bufferView\":2,\"componentType\":5123,\"type\":\"SCALAR\",\"count\":3},"
               "{\"bufferView\":3,\"componentType\":5123,\"type\":\"VEC4\",\"count\":3},"
               "{\"bufferView\":4,\"componentType\":5126,\"type\":\"VEC4\",\"count\":3},"
               "{\"bufferView\":5,\"componentType\":5126,\"type\":\"MAT4\",\"count\":1},"
               "{\"bufferView\":2,\"componentType\":5126,\"type\":\"SCALAR\",\"count\":1},"
               "{\"bufferView\":4,\"componentType\":5126,\"type\":\"VEC4\",\"count\":3}]}";

// The read callback: returns a small buffer for any URI, so external-file paths get
// fuzzed too (the loader must survive short or nonsense external data).
static uint8_t s_external[64];
static void* s_read(const char* path, int* size_out, void* udata)
{
	(void)path; (void)udata;
	*size_out = (int)(s_rand_below(sizeof(s_external)) + 1);
	return s_external;
}

int main(int argc, char** argv)
{
	// "" or "-" selects the builtin seed (some shells swallow empty arguments).
	const char* seed_path = argc > 1 && argv[1][0] && strcmp(argv[1], "-") ? argv[1] : NULL;
	int iterations = argc > 2 ? atoi(argv[2]) : 20000;
	uint64_t start_seed = argc > 3 ? (uint64_t)strtoull(argv[3], NULL, 10) : 1;

	uint8_t* seed = NULL;
	int seed_size = 0;
	if (seed_path) {
		FILE* fp = fopen(seed_path, "rb");
		if (!fp) { printf("could not open %s\n", seed_path); return 1; }
		fseek(fp, 0, SEEK_END);
		seed_size = (int)ftell(fp);
		fseek(fp, 0, SEEK_SET);
		seed = (uint8_t*)malloc((size_t)seed_size);
		if (fread(seed, 1, (size_t)seed_size, fp) != (size_t)seed_size) { printf("short read\n"); return 1; }
		fclose(fp);
	} else {
		seed_size = (int)strlen(s_builtin_seed);
		seed = (uint8_t*)malloc((size_t)seed_size);
		memcpy(seed, s_builtin_seed, (size_t)seed_size);
	}
	printf("fuzzing %s (%d bytes), %d iterations from seed %llu\n",
		seed_path ? seed_path : "<builtin glTF>", seed_size, iterations, (unsigned long long)start_seed);

	uint8_t* buf = (uint8_t*)malloc((size_t)seed_size);
	int loaded = 0, rejected = 0;
	for (int i = 0; i < iterations; ++i) {
		// Mix the (seed, iteration) pair through a splitmix-style finalizer. Adding them
		// instead would make runs at nearby start seeds explore nearly the same states.
		uint64_t z = (start_seed * 0x9E3779B97F4A7C15ULL) + (uint64_t)i * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
		s_rng_state = (z ^ (z >> 31)) | 1u; // xorshift dies at zero.
		memcpy(buf, seed, (size_t)seed_size);
		int size = seed_size;

		// One to eight mutations per case, from a mix that targets both the binary
		// container and the JSON text.
		int mutations = 1 + (int)s_rand_below(8);
		for (int m = 0; m < mutations; ++m) {
			switch (s_rand_below(6)) {
			case 0: // Bit flip.
				if (size) buf[s_rand_below((uint32_t)size)] ^= (uint8_t)(1u << s_rand_below(8));
				break;
			case 1: // Random byte.
				if (size) buf[s_rand_below((uint32_t)size)] = (uint8_t)s_rand_below(256);
				break;
			case 2: // Truncate -- the classic way to walk off the end of a chunk.
				size = (int)s_rand_below((uint32_t)size + 1);
				break;
			case 3: { // Interesting integer, hitting length/offset/count fields hard.
				static const uint32_t interesting[] = { 0u, 1u, 0x7Fu, 0x80u, 0xFFu, 0x7FFFu, 0xFFFFu,
					0x7FFFFFFFu, 0x80000000u, 0xFFFFFFFFu, 0xFFFFFFF0u };
				if (size >= 4) {
					uint32_t v = interesting[s_rand_below((uint32_t)(sizeof(interesting) / sizeof(*interesting)))];
					memcpy(buf + s_rand_below((uint32_t)size - 3), &v, 4);
				}
			}	break;
			case 4: { // JSON structural damage: swap a delimiter for another.
				static const char delims[] = "{}[]\":,0123456789.eE-";
				if (size) buf[s_rand_below((uint32_t)size)] = delims[s_rand_below((uint32_t)sizeof(delims) - 1)];
			}	break;
			case 5: { // Splice a block from elsewhere in the file onto itself.
				if (size > 8) {
					uint32_t len = 1 + s_rand_below((uint32_t)size / 4);
					uint32_t src = s_rand_below((uint32_t)size - len);
					uint32_t dst = s_rand_below((uint32_t)size - len);
					memmove(buf + dst, buf + src, len);
				}
			}	break;
			}
		}

		CM_LoadParams params;
		memset(&params, 0, sizeof(params));
		params.read_fn = s_read;
		CM_Model* model = cm_load_ex(buf, size, params);
		if (model) {
			// Exercise the runtime too: a malformed-but-accepted model must not blow up
			// downstream. Node/joint indices are the interesting attack surface here.
			loaded++;
			if (model->node_count > 0 && model->node_count < 4096) {
				CM_Transform* locals = (CM_Transform*)malloc(sizeof(CM_Transform) * (size_t)model->node_count);
				float* world = (float*)malloc(sizeof(float) * 16 * (size_t)model->node_count);
				cm_rest_pose(model, locals);
				for (int a = 0; a < model->animation_count; ++a) {
					cm_animate(model, model->animations + a, 0.25f, locals);
					float w[8] = { 0 };
					cm_animate_weights(model, model->animations + a, 0.25f, 0, w, 8);
				}
				cm_world_transforms(model, locals, world);
				for (int s = 0; s < model->skin_count; ++s) {
					int n = model->skins[s].joint_count;
					if (n > 0 && n < 4096) {
						float* palette = (float*)malloc(sizeof(float) * 16 * (size_t)n);
						cm_skin_palette(model, s, world, palette);
						free(palette);
					}
				}
				CM_Transform* blended = (CM_Transform*)malloc(sizeof(CM_Transform) * (size_t)model->node_count);
				cm_blend(model, locals, locals, 0.5f, blended);
				free(blended);
				free(locals);
				free(world);
			}
			cm_free(model);
		} else {
			rejected++;
		}
		if (((i + 1) % 5000) == 0) printf("  %d/%d (%d loaded, %d rejected)\n", i + 1, iterations, loaded, rejected);
	}

	printf("done: %d loaded, %d rejected, no crashes\n", loaded, rejected);
	free(buf);
	free(seed);
	return 0;
}
