/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
#include <cute/ckit.h>
#define CUTE_MODEL_IMPLEMENTATION
#include <cute/cute_model.h>

// cute_model loader tests: pure CPU, no app or GPU. A handcrafted .gltf with an external
// binary buffer and image exercises the read callback (with percent-decoded URIs), the
// COLOR_0 / generated-tangent attribute paths, the full material set including
// KHR_texture_transform, CUBICSPLINE Hermite evaluation, and cm_blend.

// One quad in the xy plane with uvs aligned to +x/+y, a normalized-u8 COLOR_0, and a
// CUBICSPLINE translation clip whose first key has out-tangent (0, 2, 0).
static const char* s_gltf =
R"({
	"asset": { "version": "2.0" },
	"scenes": [{ "nodes": [0] }],
	"nodes": [{ "name": "quad", "mesh": 0 }],
	"meshes": [{ "primitives": [{
		"attributes": { "POSITION": 0, "TEXCOORD_0": 1, "COLOR_0": 2 },
		"indices": 3,
		"material": 0
	}] }],
	"buffers": [{ "uri": "test%20data.bin", "byteLength": 184 }],
	"bufferViews": [
		{ "buffer": 0, "byteOffset": 0, "byteLength": 48 },
		{ "buffer": 0, "byteOffset": 48, "byteLength": 32 },
		{ "buffer": 0, "byteOffset": 80, "byteLength": 12 },
		{ "buffer": 0, "byteOffset": 92, "byteLength": 12 },
		{ "buffer": 0, "byteOffset": 104, "byteLength": 8 },
		{ "buffer": 0, "byteOffset": 112, "byteLength": 72 }
	],
	"accessors": [
		{ "bufferView": 0, "componentType": 5126, "type": "VEC3", "count": 4 },
		{ "bufferView": 1, "componentType": 5126, "type": "VEC2", "count": 4 },
		{ "bufferView": 2, "componentType": 5121, "type": "VEC3", "count": 4, "normalized": true },
		{ "bufferView": 3, "componentType": 5123, "type": "SCALAR", "count": 6 },
		{ "bufferView": 4, "componentType": 5126, "type": "SCALAR", "count": 2, "max": [1.0] },
		{ "bufferView": 5, "componentType": 5126, "type": "VEC3", "count": 6 }
	],
	"animations": [{
		"name": "clip",
		"channels": [{ "sampler": 0, "target": { "node": 0, "path": "translation" } }],
		"samplers": [{ "input": 4, "output": 5, "interpolation": "CUBICSPLINE" }]
	}],
	"materials": [{
		"name": "mat",
		"pbrMetallicRoughness": {
			"baseColorFactor": [0.5, 0.5, 0.5, 1.0],
			"metallicFactor": 0.25,
			"roughnessFactor": 0.75,
			"baseColorTexture": { "index": 0, "extensions": { "KHR_texture_transform": {
				"offset": [0.25, 0.5], "scale": [2.0, 3.0], "rotation": 1.5 } } }
		},
		"normalTexture": { "index": 0, "scale": 0.8 },
		"emissiveFactor": [1.0, 0.5, 0.25],
		"alphaMode": "MASK",
		"alphaCutoff": 0.3,
		"doubleSided": true
	}],
	"textures": [{ "source": 0 }],
	"images": [{ "uri": "tex.png" }]
})";

static int s_free_count;

static void* s_read(const char* path, int* size_out, void* udata)
{
	CF_UNUSED(udata);
	if (!CF_STRCMP(path, "test data.bin")) { // "test%20data.bin", percent-decoded.
		static float bin[46];
		char* p = (char*)bin;
		float positions[12] = { 0,0,0, 1,0,0, 1,1,0, 0,1,0 };
		float uvs[8] = { 0,0, 1,0, 1,1, 0,1 };
		uint8_t colors[12] = { 255,0,0, 0,255,0, 0,0,255, 255,255,255 };
		uint16_t indices[6] = { 0,1,2, 0,2,3 };
		float times[2] = { 0, 1 };
		// Two CUBICSPLINE keys as in-tangent/value/out-tangent triples.
		float output[18] = {
			0,0,0,  0,0,0,  0,2,0, // Key 0: value (0,0,0), out-tangent (0,2,0).
			0,0,0,  0,1,0,  0,0,0, // Key 1: value (0,1,0), in-tangent zero.
		};
		CF_MEMCPY(p, positions, 48);
		CF_MEMCPY(p + 48, uvs, 32);
		CF_MEMCPY(p + 80, colors, 12);
		CF_MEMCPY(p + 92, indices, 12);
		CF_MEMCPY(p + 104, times, 8);
		CF_MEMCPY(p + 112, output, 72);
		*size_out = 184;
		return p;
	}
	if (!CF_STRCMP(path, "tex.png")) {
		static const char png[] = "PNGDATA";
		*size_out = 7;
		return (void*)png;
	}
	return NULL;
}

static void s_free(void* data, void* udata)
{
	CF_UNUSED(data);
	CF_UNUSED(udata);
	s_free_count++;
}

TEST_CASE(test_model_gltf_external)
{
	s_free_count = 0;
	CM_LoadParams params = { 0 };
	params.read_fn = s_read;
	params.free_fn = s_free;
	CM_Model* model = cm_load_ex(s_gltf, (int)CF_STRLEN(s_gltf), params);
	REQUIRE(model);
	REQUIRE(s_free_count == 2); // Buffer + image both released back to us.

	// Vertex streams.
	REQUIRE(model->mesh_count == 1);
	CM_Primitive* prim = model->meshes[0].primitives;
	REQUIRE(prim->vertex_count == 4);
	REQUIRE(prim->index_count == 6);
	REQUIRE(prim->uvs);
	REQUIRE(prim->colors);
	REQUIRE(cf_abs(prim->colors[0] - 1.0f) < 0.01f); // (255, 0, 0) normalized...
	REQUIRE(prim->colors[1] < 0.01f);
	REQUIRE(cf_abs(prim->colors[3] - 1.0f) < 0.01f); // ...with alpha forced to 1 for VEC3.

	// Generated normals face +z (CCW quad in the xy plane); generated tangents align
	// with +x (uv u runs along +x) with positive handedness.
	REQUIRE(prim->normals && cf_abs(prim->normals[2] - 1.0f) < 0.001f);
	REQUIRE(prim->tangents);
	REQUIRE(cf_abs(prim->tangents[0] - 1.0f) < 0.001f);
	REQUIRE(cf_abs(prim->tangents[3] - 1.0f) < 0.001f);

	// Full material set + KHR_texture_transform.
	REQUIRE(model->material_count == 1);
	CM_Material* mat = model->materials;
	REQUIRE(cf_abs(mat->base_color[0] - 0.5f) < 0.001f);
	REQUIRE(cf_abs(mat->metallic - 0.25f) < 0.001f);
	REQUIRE(cf_abs(mat->roughness - 0.75f) < 0.001f);
	REQUIRE(mat->base_color_texture.image == 0);
	REQUIRE(cf_abs(mat->base_color_texture.offset[0] - 0.25f) < 0.001f);
	REQUIRE(cf_abs(mat->base_color_texture.offset[1] - 0.5f) < 0.001f);
	REQUIRE(cf_abs(mat->base_color_texture.scale[0] - 2.0f) < 0.001f);
	REQUIRE(cf_abs(mat->base_color_texture.scale[1] - 3.0f) < 0.001f);
	REQUIRE(cf_abs(mat->base_color_texture.rotation - 1.5f) < 0.001f);
	REQUIRE(mat->normal_texture.image == 0);
	REQUIRE(cf_abs(mat->normal_scale - 0.8f) < 0.001f);
	REQUIRE(mat->metallic_roughness_texture.image == -1);
	REQUIRE(cf_abs(mat->emissive[0] - 1.0f) < 0.001f && cf_abs(mat->emissive[2] - 0.25f) < 0.001f);
	REQUIRE(mat->alpha_mode == CM_ALPHA_MASK);
	REQUIRE(cf_abs(mat->alpha_cutoff - 0.3f) < 0.001f);
	REQUIRE(mat->double_sided);

	// External image bytes round-tripped through the callback.
	REQUIRE(model->image_count == 1);
	REQUIRE(model->images[0].size == 7);
	REQUIRE(!CF_MEMCMP(model->images[0].data, "PNGDATA", 7));

	// CUBICSPLINE Hermite: at t = 0.5 with out-tangent (0, 2, 0) the y lane lands at
	// h00*0 + h10*2 + h01*1 + h11*0 = 0.125*2 + 0.5 = 0.75, not the linear 0.5.
	REQUIRE(model->animation_count == 1);
	CM_Animation* clip = cm_find_animation(model, "clip");
	REQUIRE(clip && clip->channel_count == 1);
	REQUIRE(clip->channels[0].interpolation == CM_INTERP_CUBIC);
	REQUIRE(cf_abs(clip->duration - 1.0f) < 0.001f);
	CM_Transform locals[1];
	cm_rest_pose(model, locals);
	cm_animate(model, clip, 0.5f, locals);
	REQUIRE(cf_abs(locals[0].translation[1] - 0.75f) < 0.001f);
	cm_animate(model, clip, 2.0f, locals); // Clamped past the end.
	REQUIRE(cf_abs(locals[0].translation[1] - 1.0f) < 0.001f);

	// cm_blend: halfway between rest (y = 0) and the clamped pose (y = 1).
	CM_Transform rest[1], blended[1];
	cm_rest_pose(model, rest);
	cm_blend(model, rest, locals, 0.5f, blended);
	REQUIRE(cf_abs(blended[0].translation[1] - 0.5f) < 0.001f);

	cm_free(model);
	return true;
}

TEST_SUITE(test_model)
{
	RUN_TEST_CASE(test_model_gltf_external);
}
