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
	"extensions": { "KHR_lights_punctual": { "lights": [{
		"name": "key", "type": "spot", "color": [1.0, 0.0, 0.0], "intensity": 2.0,
		"range": 10.0, "spot": { "innerConeAngle": 0.2, "outerConeAngle": 0.5 }
	}] } },
	"nodes": [{
		"name": "quad", "mesh": 0,
		"extensions": { "KHR_lights_punctual": { "light": 0 } },
		"extras": { "hp": 5, "tag": "boss" }
	}],
	"meshes": [{
		"primitives": [{
			"attributes": { "POSITION": 0, "TEXCOORD_0": 1, "TEXCOORD_1": 1, "COLOR_0": 2 },
			"indices": 3,
			"material": 0,
			"targets": [{ "POSITION": 6 }]
		}],
		"weights": [0.25]
	}],
	"buffers": [{ "uri": "test%20data.bin", "byteLength": 208 }],
	"bufferViews": [
		{ "buffer": 0, "byteOffset": 0, "byteLength": 48 },
		{ "buffer": 0, "byteOffset": 48, "byteLength": 32 },
		{ "buffer": 0, "byteOffset": 80, "byteLength": 12 },
		{ "buffer": 0, "byteOffset": 92, "byteLength": 12 },
		{ "buffer": 0, "byteOffset": 104, "byteLength": 8 },
		{ "buffer": 0, "byteOffset": 112, "byteLength": 72 },
		{ "buffer": 0, "byteOffset": 184, "byteLength": 2 },
		{ "buffer": 0, "byteOffset": 188, "byteLength": 12 },
		{ "buffer": 0, "byteOffset": 200, "byteLength": 8 }
	],
	"accessors": [
		{ "bufferView": 0, "componentType": 5126, "type": "VEC3", "count": 4 },
		{ "bufferView": 1, "componentType": 5126, "type": "VEC2", "count": 4 },
		{ "bufferView": 2, "componentType": 5121, "type": "VEC3", "count": 4, "normalized": true },
		{ "bufferView": 3, "componentType": 5123, "type": "SCALAR", "count": 6 },
		{ "bufferView": 4, "componentType": 5126, "type": "SCALAR", "count": 2, "max": [1.0] },
		{ "bufferView": 5, "componentType": 5126, "type": "VEC3", "count": 6 },
		{ "componentType": 5126, "type": "VEC3", "count": 4, "sparse": {
			"count": 1,
			"indices": { "bufferView": 6, "componentType": 5123 },
			"values": { "bufferView": 7 }
		} },
		{ "bufferView": 8, "componentType": 5126, "type": "SCALAR", "count": 2 }
	],
	"animations": [{
		"name": "clip",
		"channels": [
			{ "sampler": 0, "target": { "node": 0, "path": "translation" } },
			{ "sampler": 1, "target": { "node": 0, "path": "weights" } }
		],
		"samplers": [
			{ "input": 4, "output": 5, "interpolation": "CUBICSPLINE" },
			{ "input": 4, "output": 7 }
		]
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
		"normalTexture": { "index": 0, "scale": 0.8, "texCoord": 1 },
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
		static float bin[52];
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
		uint16_t sparse_index[1] = { 2 };
		float sparse_value[3] = { 0, 0, 1 };  // Morph delta at vertex 2 only.
		float weight_keys[2] = { 0, 1 };      // Morph weight ramps 0 -> 1 over the clip.
		CF_MEMCPY(p, positions, 48);
		CF_MEMCPY(p + 48, uvs, 32);
		CF_MEMCPY(p + 80, colors, 12);
		CF_MEMCPY(p + 92, indices, 12);
		CF_MEMCPY(p + 104, times, 8);
		CF_MEMCPY(p + 112, output, 72);
		CF_MEMCPY(p + 184, sparse_index, 2);
		CF_MEMCPY(p + 188, sparse_value, 12);
		CF_MEMCPY(p + 200, weight_keys, 8);
		*size_out = 208;
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

	// Bounds are computed from the position stream at load: the unit quad spans
	// (0,0,0)..(1,1,0) regardless of what any accessor min/max json claims.
	REQUIRE(prim->aabb_min[0] == 0 && prim->aabb_min[1] == 0 && prim->aabb_min[2] == 0);
	REQUIRE(prim->aabb_max[0] == 1.0f && prim->aabb_max[1] == 1.0f && prim->aabb_max[2] == 0);

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
	REQUIRE(clip && clip->channel_count == 2); // Translation + morph weights.
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

	// TEXCOORD_1 stream + per-slot uv set selection.
	REQUIRE(prim->uvs1);
	REQUIRE(mat->normal_texture.uv_set == 1);
	REQUIRE(mat->base_color_texture.uv_set == 0);

	// Extras passthrough: raw JSON, ours to parse.
	REQUIRE(model->nodes[0].extras);
	REQUIRE(strstr(model->nodes[0].extras, "\"tag\""));
	REQUIRE(strstr(model->nodes[0].extras, "boss"));

	// Sparse-encoded morph target: a (0, 0, 1) position delta at vertex 2 only.
	REQUIRE(prim->target_count == 1);
	REQUIRE(prim->targets[0].positions);
	REQUIRE(cf_abs(prim->targets[0].positions[2 * 3 + 2] - 1.0f) < 0.001f);
	REQUIRE(cf_abs(prim->targets[0].positions[0]) < 0.001f);
	REQUIRE(!prim->targets[0].normals);

	// Mesh default weights, then the animated weights channel ramping 0 -> 1.
	REQUIRE(model->meshes[0].weight_count == 1);
	REQUIRE(cf_abs(model->meshes[0].weights[0] - 0.25f) < 0.001f);
	float morph_w[1] = { model->meshes[0].weights[0] };
	cm_animate_weights(model, clip, 0.5f, 0, morph_w, 1);
	REQUIRE(cf_abs(morph_w[0] - 0.5f) < 0.001f);

	// Designer-placed spot light, referenced by the node.
	REQUIRE(model->light_count == 1);
	REQUIRE(model->lights[0].type == CM_LIGHT_SPOT);
	REQUIRE(cf_abs(model->lights[0].color[0] - 1.0f) < 0.001f && model->lights[0].color[1] < 0.001f);
	REQUIRE(cf_abs(model->lights[0].intensity - 2.0f) < 0.001f);
	REQUIRE(cf_abs(model->lights[0].range - 10.0f) < 0.001f);
	REQUIRE(cf_abs(model->lights[0].inner_cone - 0.2f) < 0.001f);
	REQUIRE(cf_abs(model->lights[0].outer_cone - 0.5f) < 0.001f);
	REQUIRE(model->nodes[0].light == 0);

	// Node lookup for attachments.
	REQUIRE(cm_find_node(model, "quad") == 0);
	REQUIRE(cm_find_node(model, "nope") == -1);

	cm_free(model);
	return true;
}

// Hostile-input regressions, all found by tools/cute_model_fuzz.c. Every case here
// crashed or read out of bounds before the hardening pass; they must now be rejected
// cleanly (NULL + an error string), never accepted with a dangling index.
TEST_CASE(test_model_malformed_rejected)
{
	// A GLB whose chunk length wraps 32-bit arithmetic: the walk must not step outside
	// the buffer looking for the next chunk header.
	uint8_t glb[64];
	CF_MEMSET(glb, 0, sizeof(glb));
	CF_MEMCPY(glb, "glTF", 4);
	uint32_t version = 2, total = (uint32_t)sizeof(glb);
	CF_MEMCPY(glb + 4, &version, 4);
	CF_MEMCPY(glb + 8, &total, 4);
	uint32_t chunk_size = 0xFFFFFFFFu, chunk_type = 0x4E4F534Au; // "JSON", absurd length.
	CF_MEMCPY(glb + 12, &chunk_size, 4);
	CF_MEMCPY(glb + 16, &chunk_type, 4);
	REQUIRE(!cm_load(glb, (int)sizeof(glb)));
	REQUIRE(cm_error()[0]);

	// An accessor whose stride * count overflows int: the span check must catch it.
	const char* overflow_accessor =
		"{\"asset\":{\"version\":\"2.0\"},"
		"\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0}}]}],"
		"\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"type\":\"VEC3\",\"count\":2000000,\"byteOffset\":0}],"
		"\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":12,\"byteStride\":2000000}],"
		"\"buffers\":[{\"uri\":\"data:application/octet-stream;base64,AAAAAAAAAAAAAAAA\",\"byteLength\":12}]}";
	REQUIRE(!cm_load(overflow_accessor, (int)CF_STRLEN(overflow_accessor)));

	// Negative offsets are nonsense and must be rejected rather than indexed with.
	const char* negative_offset =
		"{\"asset\":{\"version\":\"2.0\"},"
		"\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0}}]}],"
		"\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"type\":\"VEC3\",\"count\":1,\"byteOffset\":-64}],"
		"\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":12}],"
		"\"buffers\":[{\"uri\":\"data:application/octet-stream;base64,AAAAAAAAAAAAAAAA\",\"byteLength\":12}]}";
	REQUIRE(!cm_load(negative_offset, (int)CF_STRLEN(negative_offset)));

	// A skin joint pointing past the node array: cm_skin_palette would index `world`
	// out of bounds, so the load must fail instead.
	const char* bad_joint =
		"{\"asset\":{\"version\":\"2.0\"},"
		"\"nodes\":[{\"name\":\"a\"}],"
		"\"skins\":[{\"joints\":[99]}]}";
	REQUIRE(!cm_load(bad_joint, (int)CF_STRLEN(bad_joint)));

	// A node listed as its own child, and one listed under two parents: the topological
	// walk must not write past the order array.
	const char* bad_hierarchy =
		"{\"asset\":{\"version\":\"2.0\"},"
		"\"nodes\":[{\"children\":[0,1,2]},{\"children\":[2]},{}]}";
	CM_Model* model = cm_load(bad_hierarchy, (int)CF_STRLEN(bad_hierarchy));
	REQUIRE(model); // Self-reference dropped, duplicate child ignored: still a valid tree.
	REQUIRE(model->node_count == 3);
	cm_free(model);

	// Dangling cross-references degrade to "absent" rather than reaching the caller.
	const char* dangling =
		"{\"asset\":{\"version\":\"2.0\"},"
		"\"nodes\":[{\"mesh\":7,\"skin\":3,\"extensions\":{\"KHR_lights_punctual\":{\"light\":5}}}]}";
	model = cm_load(dangling, (int)CF_STRLEN(dangling));
	REQUIRE(model);
	REQUIRE(model->nodes[0].mesh == -1);
	REQUIRE(model->nodes[0].skin == -1);
	REQUIRE(model->nodes[0].light == -1);
	cm_free(model);

	return true;
}

TEST_SUITE(test_model)
{
	RUN_TEST_CASE(test_model_gltf_external);
	RUN_TEST_CASE(test_model_malformed_rejected);
}
