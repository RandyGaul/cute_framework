/*
	------------------------------------------------------------------------------
		Licensing information can be found at the end of the file.
	------------------------------------------------------------------------------

	cute_model.h - v1.00

	To create implementation (the function definitions)
		#define CUTE_MODEL_IMPLEMENTATION
	in *one* C/CPP file (translation unit) that includes this file

	HARD DEPENDENCY

		This header requires cute/ckit.h (strings, interning, dynamic arrays) to be
		included *before* it, and CKIT_IMPLEMENTATION must be provided by some TU in
		the program (Cute Framework's cute.lib already provides it):

			#include "cute/ckit.h"
			#define CUTE_MODEL_IMPLEMENTATION
			#include "cute/cute_model.h"

	SUMMARY

		cute_model.h is a single-file glTF 2.0 / GLB loader for game runtimes. It
		reads a model from memory and hands back plain arrays: de-indexed-friendly
		vertex streams (positions, normals, uvs, joints, weights, indices), a node
		hierarchy with rest-pose transforms, skins with inverse-bind matrices,
		keyframe animation clips, and embedded images -- everything needed to feed
		a GPU skinning pipeline, with zero opinions about how you render.

		The runtime half samples clips and composes matrix palettes:

			CM_Model* model = cm_load(data, size);
			CM_Animation* run = cm_find_animation(model, "Run");

			// Per frame:
			cm_rest_pose(model, locals);                       // locals: node_count transforms
			cm_animate(model, run, fmodf(t, run->duration), locals);
			cm_world_transforms(model, locals, world);         // world: node_count mat4s
			cm_skin_palette(model, 0, world, palette);         // palette: joint_count mat4s

		Palette matrices arrive fully composed (posed * inverse-bind), column-major,
		ready to upload straight into a mat4 array uniform -- the standard skinning
		contract: the vertex shader only does the weighted blend. Per the glTF spec
		the skinned mesh's own node transform is ignored; render the mesh under
		whatever model transform you like.

	FORMAT SUPPORT

		- GLB (binary container) with an embedded buffer, and plain .gltf JSON with
		  base64 data-URI buffers. External file references (.bin buffers, image
		  files) resolve through the read callback in CM_LoadParams -- this header
		  does no file IO of its own. Without a callback, only self-contained files
		  load.
		- Attributes: POSITION (required), NORMAL (generated smooth if absent),
		  TANGENT (generated from uvs + normals if absent), TEXCOORD_0/TEXCOORD_1
		  and COLOR_0 (float / normalized u8 / normalized u16), JOINTS_0 (u8 / u16),
		  WEIGHTS_0 (float / normalized u8 / normalized u16).
		- Indices u8/u16/u32 (widened to u32); unindexed primitives get 0..n-1.
		- Sparse accessors (morph targets' usual encoding) for float streams.
		- Morph targets: position/normal/tangent delta streams per primitive, mesh
		  default weights, and animated weights channels (cm_animate_weights).
		- Animations: translation/rotation/scale/weights channels with STEP, LINEAR
		  (shortest-path slerp for rotations), and CUBICSPLINE (proper Hermite
		  evaluation) interpolation.
		- Node `matrix` transforms are decomposed to TRS (assumes no shear).
		- Materials: the full pbrMetallicRoughness set (base color, metallic/
		  roughness, normal, occlusion, emissive factors and textures), alpha
		  mode/cutoff, double-sided, per-slot TEXCOORD set selection, and
		  KHR_texture_transform per texture slot.
		- KHR_lights_punctual: designer-placed lights as plain data on nodes.
		- Raw "extras" JSON passes through on nodes, meshes, and materials --
		  Blender custom properties export there, so gameplay markup authored in
		  the DCC (spawn points, sockets, flags) arrives intact for you to parse.
		- Not supported: Draco/meshopt compression, KHR_materials_* extensions
		  (sheen, clearcoat, ...), sparse integer streams (joints/indices).

	Generated tangents use area-weighted accumulation (Lengyel), orthonormalized
	against the normal -- good for typical assets, but bake-critical normal maps
	should ship authored (MikkTSpace) tangents in the file.

	All matrices are column-major float[16], matching GLSL and Cute Framework.
*/

#ifndef CUTE_MODEL_H
#define CUTE_MODEL_H

#ifndef CKIT_H
#	error "cute_model.h requires cute/ckit.h to be included beforehand."
#endif

#include <stdint.h>

//--------------------------------------------------------------------------------------------------
// Data types.

// A local transform as translation/rotation(quat xyzw)/scale.
typedef struct CM_Transform
{
	float translation[3];
	float rotation[4];
	float scale[3];
} CM_Transform;

// One drawable vertex stream (a glTF primitive). Streams are separate tightly-packed
// arrays; interleave (or don't) however your renderer likes.
// One morph target's delta streams (glTF primitive targets). Each non-NULL array holds
// 3 floats per vertex to add, scaled by the target's weight, onto the base attribute.
typedef struct CM_MorphTarget
{
	float* positions;
	float* normals;
	float* tangents; // xyz deltas only; handedness never morphs.
} CM_MorphTarget;

typedef struct CM_Primitive
{
	int vertex_count;
	float* positions;   // 3 floats per vertex. Never NULL.
	float* normals;     // 3 floats per vertex. Generated (smooth) when the file has none.
	float* tangents;    // 4 floats per vertex (xyz + w handedness sign). Generated from
	                    // uvs + normals when the file has none; NULL without uvs.
	float* uvs;         // 2 floats per vertex (TEXCOORD_0), or NULL.
	float* uvs1;        // 2 floats per vertex (TEXCOORD_1, lightmaps/AO), or NULL.
	float* colors;      // 4 floats per vertex (COLOR_0), or NULL.
	uint16_t* joints;   // 4 per vertex, or NULL for unskinned meshes.
	float* weights;     // 4 per vertex, or NULL for unskinned meshes.
	int index_count;
	uint32_t* indices;  // Never NULL (generated 0..n-1 for unindexed primitives).
	int material;       // Index into model->materials, or -1.
	int target_count;   // Morph targets (blendshapes).
	CM_MorphTarget* targets;
	float aabb_min[3];  // Position bounds, computed from the stream at load (rest pose;
	float aabb_max[3];  // morphs/skinning move vertices, so pad accordingly for culling).
} CM_Primitive;

typedef struct CM_Mesh
{
	const char* name;    // Interned; "" if unnamed.
	int primitive_count;
	CM_Primitive* primitives;
	float* weights;      // Default morph weights (target_count of them), or NULL for zeros.
	int weight_count;
	const char* extras;  // Raw JSON of the glTF "extras" object, or NULL.
} CM_Mesh;

typedef struct CM_Node
{
	const char* name;   // Interned; "" if unnamed.
	int parent;         // Node index, or -1 for roots.
	int* children;
	int child_count;
	int mesh;           // Index into model->meshes, or -1.
	int skin;           // Index into model->skins, or -1.
	int light;          // Index into model->lights (KHR_lights_punctual), or -1.
	CM_Transform rest;  // Local rest-pose transform.
	const char* extras; // Raw JSON of the glTF "extras" object -- Blender custom
	                    // properties land here -- or NULL. Parse it however you like.
} CM_Node;

// A designer-placed light (KHR_lights_punctual): position and orientation come from the
// owning node's world transform (lights aim down the node's -z, per the spec).
typedef enum CM_LightType
{
	CM_LIGHT_DIRECTIONAL,
	CM_LIGHT_POINT,
	CM_LIGHT_SPOT,
} CM_LightType;

typedef struct CM_Light
{
	const char* name;  // Interned; "" if unnamed.
	CM_LightType type;
	float color[3];    // Linear RGB, defaults to white.
	float intensity;   // Candela (point/spot) or lux (directional); default 1.
	float range;       // 0 means unbounded.
	float inner_cone;  // Spot angles in radians.
	float outer_cone;
} CM_Light;

typedef struct CM_Skin
{
	const char* name;      // Interned; "" if unnamed.
	int joint_count;
	int* joints;           // Node indices, palette order.
	float* inverse_binds;  // joint_count column-major mat4s.
} CM_Skin;

typedef enum CM_ChannelPath
{
	CM_PATH_TRANSLATION,
	CM_PATH_ROTATION,
	CM_PATH_SCALE,
	CM_PATH_WEIGHTS, // Morph target weights; sampled via cm_animate_weights.
} CM_ChannelPath;

typedef enum CM_Interpolation
{
	CM_INTERP_LINEAR,
	CM_INTERP_STEP,
	CM_INTERP_CUBIC, // glTF CUBICSPLINE; see CM_Channel::values for the key layout.
} CM_Interpolation;

typedef struct CM_Channel
{
	int node;                     // Target node index.
	CM_ChannelPath path;
	CM_Interpolation interpolation;
	int component_count;          // Floats per element: 3 (T/S), 4 (R), morph target count (weights).
	int key_count;
	float* times;                 // Ascending keyframe times in seconds.
	float* values;                // One element (component_count floats) per key.
	                              // CM_INTERP_CUBIC keys hold glTF's in-tangent/value/
	                              // out-tangent triples: 3 elements per key, value in the
	                              // middle. The sampling functions handle both layouts.
} CM_Channel;

typedef struct CM_Animation
{
	const char* name;   // Interned; "" if unnamed.
	float duration;     // Largest keyframe time across channels, in seconds.
	int channel_count;
	CM_Channel* channels;
} CM_Animation;

// An embedded image, still encoded (PNG/JPEG bytes) -- decode with your image library.
typedef struct CM_Image
{
	const char* name;      // Interned; "" if unnamed.
	const char* mime_type; // Interned, e.g. "image/png"; "" if unspecified.
	uint8_t* data;
	int size;
} CM_Image;

// One material texture slot: an image index, which TEXCOORD set it samples, and its
// KHR_texture_transform (identity when the file carries none). Apply as
// uv' = offset + rotate(rotation) * (uv * scale).
typedef struct CM_TextureRef
{
	int image;       // Index into model->images, or -1 when the slot is unused.
	int uv_set;      // 0 samples CM_Primitive::uvs, 1 samples uvs1.
	float offset[2];
	float scale[2];  // Defaults to (1, 1).
	float rotation;  // Radians, clockwise about the uv origin per the extension spec.
} CM_TextureRef;

typedef enum CM_AlphaMode
{
	CM_ALPHA_OPAQUE,
	CM_ALPHA_MASK,   // Cut out at alpha_cutoff.
	CM_ALPHA_BLEND,
} CM_AlphaMode;

// The full pbrMetallicRoughness set, as data -- what your renderer does with it is its
// business. Unused texture slots have image == -1; factors always hold spec defaults.
typedef struct CM_Material
{
	const char* name;             // Interned; "" if unnamed.
	float base_color[4];          // Base color factor (defaults to white).
	CM_TextureRef base_color_texture;
	float metallic;               // Factor, default 1.
	float roughness;              // Factor, default 1.
	CM_TextureRef metallic_roughness_texture; // G = roughness, B = metallic, per spec.
	CM_TextureRef normal_texture;
	float normal_scale;           // Default 1.
	CM_TextureRef occlusion_texture;          // R channel, per spec.
	float occlusion_strength;     // Default 1.
	float emissive[3];            // Emissive factor, default black.
	CM_TextureRef emissive_texture;
	CM_AlphaMode alpha_mode;
	float alpha_cutoff;           // Default 0.5; meaningful under CM_ALPHA_MASK.
	int double_sided;
	const char* extras;           // Raw JSON of the glTF "extras" object, or NULL.
} CM_Material;

typedef struct CM_Model
{
	int node_count;      CM_Node* nodes;
	int mesh_count;      CM_Mesh* meshes;
	int skin_count;      CM_Skin* skins;
	int animation_count; CM_Animation* animations;
	int image_count;     CM_Image* images;
	int material_count;  CM_Material* materials;
	int light_count;     CM_Light* lights;
	int* order;          // node_count node indices, parents always before children.
} CM_Model;

//--------------------------------------------------------------------------------------------------
// Loading.

// Optional hooks for cm_load_ex. cute_model does no file IO of its own; when a .gltf
// references external files (a .bin buffer, image files), each URI -- percent-decoded,
// relative to the model file -- is handed to read_fn. Return the file's bytes from any
// allocator; free_fn (when non-NULL) is called once cute_model has copied what it needs.
typedef struct CM_LoadParams
{
	void* (*read_fn)(const char* path, int* size_out, void* udata);
	void (*free_fn)(void* data, void* udata);
	void* udata;
} CM_LoadParams;

// Loads a GLB or .gltf model from memory. Returns NULL on failure -- call cm_error()
// for a human-readable reason. The returned model owns all of its arrays; free the
// whole thing with cm_free.
CM_Model* cm_load(const void* data, int size);

// cm_load with external-file resolution (see CM_LoadParams).
CM_Model* cm_load_ex(const void* data, int size, CM_LoadParams params);

// The reason the last cm_load returned NULL. Static storage, valid until the next call.
const char* cm_error(void);

void cm_free(CM_Model* model);

// Finds an animation by (case-sensitive) name, or NULL.
CM_Animation* cm_find_animation(const CM_Model* model, const char* name);

// Finds a node by (case-sensitive) name -- attach props to bones, look up authored
// markers, etc. Returns the node index, or -1.
int cm_find_node(const CM_Model* model, const char* name);

//--------------------------------------------------------------------------------------------------
// Runtime. All output arrays are caller-allocated:
//   locals  -- model->node_count CM_Transforms.
//   world   -- model->node_count mat4s (16 floats each), column-major.
//   palette -- skin->joint_count mat4s, column-major.

// Fills `locals` with every node's rest-pose transform.
void cm_rest_pose(const CM_Model* model, CM_Transform* locals);

// Samples `animation` at `time` (seconds, clamped to the clip; loop with fmodf yourself)
// on top of `locals`: only animated paths are overwritten, so un-animated nodes keep
// whatever `locals` already holds (typically the rest pose). Call multiple times with
// different clips to layer partial animations.
void cm_animate(const CM_Model* model, const CM_Animation* animation, float time, CM_Transform* locals);

// Composes local transforms down the hierarchy into world-space matrices.
void cm_world_transforms(const CM_Model* model, const CM_Transform* locals, float* world);

// Writes skin_index's matrix palette: palette[j] = world[joint j] * inverse_bind[j].
// Upload straight into a mat4 array uniform; the vertex shader does the weighted blend.
// Per the glTF spec the skinned mesh's node transform is NOT included -- render the
// mesh under any model transform you like.
void cm_skin_palette(const CM_Model* model, int skin_index, const float* world, float* palette);

// Blends two whole poses: out = lerp(a, b, t) per node, with shortest-path slerp for
// rotations. Sample two clips into separate local arrays and blend for crossfades.
void cm_blend(const CM_Model* model, const CM_Transform* a, const CM_Transform* b, float t, CM_Transform* out);

// Samples `animation`'s morph-weight channels targeting `node` into weights (up to
// weight_count entries, matching the mesh's target_count). Entries stay untouched when
// the animation has no weights channel for the node, so prefill with the mesh's default
// weights (CM_Mesh::weights) or zeros. Apply as: vertex += sum(weights[i] * delta[i]).
void cm_animate_weights(const CM_Model* model, const CM_Animation* animation, float time, int node, float* weights, int weight_count);

#endif // CUTE_MODEL_H

//--------------------------------------------------------------------------------------------------

#ifdef CUTE_MODEL_IMPLEMENTATION
#ifndef CUTE_MODEL_IMPLEMENTATION_ONCE
#define CUTE_MODEL_IMPLEMENTATION_ONCE

#include <stdlib.h>
#include <string.h>
#include <math.h>

#if !defined(CM_ALLOC)
#	define CM_ALLOC(size) malloc(size)
#	define CM_FREE(ptr) free(ptr)
#endif

static char cm_g_error[256];
const char* cm_error(void) { return cm_g_error; }
#define CM_FAIL(...) do { snprintf(cm_g_error, sizeof(cm_g_error), __VA_ARGS__); return 0; } while (0)

//--------------------------------------------------------------------------------------------------
// Minimal JSON DOM. glTF JSON is machine-written and well-formed; this parser is strict
// enough to reject garbage and small enough to audit. Values live in one stretchy pool,
// strings are interned, object member lookup is a linear scan (glTF objects are small).

typedef struct CM_JVal
{
	char kind; // 'o' object, 'a' array, 's' string, 'n' number, 'b' bool, 'z' null.
	double num;
	const char* str;
	int* items;           // ckit array of value indices (arrays and objects).
	const char** keys;    // ckit array of interned member names, parallel to items (objects).
	const char* src_begin; // Source span of this value, for raw "extras" passthrough.
	const char* src_end;
} CM_JVal;

typedef struct CM_Json
{
	CM_JVal* vals; // ckit array; index 0 is the root after a successful parse.
	const char* p;
	const char* end;
} CM_Json;

static void cm_json_free(CM_Json* j)
{
	for (int i = 0; i < asize(j->vals); ++i) {
		afree(j->vals[i].items);
		afree(j->vals[i].keys);
	}
	afree(j->vals);
}

static void cm_jskip_ws(CM_Json* j)
{
	while (j->p < j->end && (*j->p == ' ' || *j->p == '\t' || *j->p == '\n' || *j->p == '\r')) j->p++;
}

static int cm_jparse_value(CM_Json* j); // Returns value index + 1, or 0 on failure.
static int cm_jparse_value_inner(CM_Json* j);

// Wraps the parser proper to record each value's source span (raw "extras" passthrough).
static int cm_jparse_value(CM_Json* j)
{
	cm_jskip_ws(j);
	const char* begin = j->p;
	int r = cm_jparse_value_inner(j);
	if (r) {
		j->vals[r - 1].src_begin = begin;
		j->vals[r - 1].src_end = j->p;
	}
	return r;
}

static int cm_jparse_string(CM_Json* j, const char** out)
{
	// j->p points at the opening quote. Escapes decode into a scratch ckit string.
	char* s = NULL;
	j->p++;
	while (j->p < j->end && *j->p != '"') {
		char c = *j->p++;
		if (c == '\\') {
			if (j->p >= j->end) { afree(s); return 0; }
			char e = *j->p++;
			switch (e) {
			case '"': apush(s, '"'); break;
			case '\\': apush(s, '\\'); break;
			case '/': apush(s, '/'); break;
			case 'b': apush(s, '\b'); break;
			case 'f': apush(s, '\f'); break;
			case 'n': apush(s, '\n'); break;
			case 'r': apush(s, '\r'); break;
			case 't': apush(s, '\t'); break;
			case 'u': {
				if (j->end - j->p < 4) { afree(s); return 0; }
				unsigned cp = 0;
				for (int i = 0; i < 4; ++i) {
					char h = *j->p++;
					cp <<= 4;
					if (h >= '0' && h <= '9') cp |= (unsigned)(h - '0');
					else if (h >= 'a' && h <= 'f') cp |= (unsigned)(h - 'a' + 10);
					else if (h >= 'A' && h <= 'F') cp |= (unsigned)(h - 'A' + 10);
					else { afree(s); return 0; }
				}
				// BMP-only UTF-8 encode; surrogate pairs land as two replacement-ish chars,
				// fine for the names glTF files carry.
				if (cp < 0x80) apush(s, (char)cp);
				else if (cp < 0x800) { apush(s, (char)(0xC0 | (cp >> 6))); apush(s, (char)(0x80 | (cp & 63))); }
				else { apush(s, (char)(0xE0 | (cp >> 12))); apush(s, (char)(0x80 | ((cp >> 6) & 63))); apush(s, (char)(0x80 | (cp & 63))); }
			}	break;
			default: afree(s); return 0;
			}
		} else {
			apush(s, c);
		}
	}
	if (j->p >= j->end) { afree(s); return 0; }
	j->p++; // Closing quote.
	apush(s, '\0');
	*out = sintern(s ? s : "");
	afree(s);
	return 1;
}

static int cm_jparse_value_inner(CM_Json* j)
{
	cm_jskip_ws(j);
	if (j->p >= j->end) return 0;
	int index = asize(j->vals);
	{
		CM_JVal v;
		memset(&v, 0, sizeof(v));
		apush(j->vals, v);
	}
	char c = *j->p;
	if (c == '{') {
		j->vals[index].kind = 'o';
		j->p++;
		cm_jskip_ws(j);
		if (j->p < j->end && *j->p == '}') { j->p++; return index + 1; }
		for (;;) {
			cm_jskip_ws(j);
			if (j->p >= j->end || *j->p != '"') return 0;
			const char* key;
			if (!cm_jparse_string(j, &key)) return 0;
			cm_jskip_ws(j);
			if (j->p >= j->end || *j->p != ':') return 0;
			j->p++;
			int child = cm_jparse_value(j);
			if (!child) return 0;
			apush(j->vals[index].keys, key);
			apush(j->vals[index].items, child - 1);
			cm_jskip_ws(j);
			if (j->p >= j->end) return 0;
			if (*j->p == ',') { j->p++; continue; }
			if (*j->p == '}') { j->p++; return index + 1; }
			return 0;
		}
	} else if (c == '[') {
		j->vals[index].kind = 'a';
		j->p++;
		cm_jskip_ws(j);
		if (j->p < j->end && *j->p == ']') { j->p++; return index + 1; }
		for (;;) {
			int child = cm_jparse_value(j);
			if (!child) return 0;
			apush(j->vals[index].items, child - 1);
			cm_jskip_ws(j);
			if (j->p >= j->end) return 0;
			if (*j->p == ',') { j->p++; continue; }
			if (*j->p == ']') { j->p++; return index + 1; }
			return 0;
		}
	} else if (c == '"') {
		j->vals[index].kind = 's';
		if (!cm_jparse_string(j, &j->vals[index].str)) return 0;
		return index + 1;
	} else if (c == 't') {
		if (j->end - j->p < 4 || memcmp(j->p, "true", 4)) return 0;
		j->vals[index].kind = 'b';
		j->vals[index].num = 1;
		j->p += 4;
		return index + 1;
	} else if (c == 'f') {
		if (j->end - j->p < 5 || memcmp(j->p, "false", 5)) return 0;
		j->vals[index].kind = 'b';
		j->p += 5;
		return index + 1;
	} else if (c == 'n') {
		if (j->end - j->p < 4 || memcmp(j->p, "null", 4)) return 0;
		j->vals[index].kind = 'z';
		j->p += 4;
		return index + 1;
	} else if (c == '-' || (c >= '0' && c <= '9')) {
		char* stop = NULL;
		j->vals[index].kind = 'n';
		j->vals[index].num = strtod(j->p, &stop);
		if (stop == j->p || stop > j->end) return 0;
		j->p = stop;
		return index + 1;
	}
	return 0;
}

static int cm_json_parse(CM_Json* j, const char* text, int size)
{
	memset(j, 0, sizeof(*j));
	j->p = text;
	j->end = text + size;
	int root = cm_jparse_value(j);
	return root == 1; // Root must be the first value parsed.
}

// Lookup helpers. `v` indices are into j->vals; -1 means absent.
static int cm_jget(const CM_Json* j, int v, const char* key)
{
	if (v < 0 || j->vals[v].kind != 'o') return -1;
	const char* k = sintern(key);
	const CM_JVal* o = j->vals + v;
	for (int i = 0; i < asize(o->keys); ++i) {
		if (o->keys[i] == k) return o->items[i];
	}
	return -1;
}
static int cm_jlen(const CM_Json* j, int v) { return v < 0 ? 0 : asize(j->vals[v].items); }
static int cm_jat(const CM_Json* j, int v, int i) { return (v < 0 || i < 0 || i >= asize(j->vals[v].items)) ? -1 : j->vals[v].items[i]; }
static double cm_jnum(const CM_Json* j, int v, double default_value) { return (v >= 0 && (j->vals[v].kind == 'n' || j->vals[v].kind == 'b')) ? j->vals[v].num : default_value; }
static int cm_jint(const CM_Json* j, int v, int default_value) { return (v >= 0 && (j->vals[v].kind == 'n' || j->vals[v].kind == 'b')) ? (int)j->vals[v].num : default_value; }
static const char* cm_jstr(const CM_Json* j, int v, const char* default_value) { return (v >= 0 && j->vals[v].kind == 's') ? j->vals[v].str : default_value; }
static int cm_jget_int(const CM_Json* j, int v, const char* key, int default_value) { return cm_jint(j, cm_jget(j, v, key), default_value); }

//--------------------------------------------------------------------------------------------------
// Buffers: the GLB binary chunk, or base64 data URIs from .gltf JSON.

static uint8_t* cm_base64_decode(const char* s, int* out_size)
{
	static const int8_t T[256] = {
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
		-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	};
	int len = (int)strlen(s);
	uint8_t* out = (uint8_t*)CM_ALLOC((size_t)(len / 4 + 1) * 3);
	int n = 0;
	unsigned acc = 0;
	int bits = 0;
	for (int i = 0; i < len; ++i) {
		int8_t d = T[(uint8_t)s[i]];
		if (d < 0) continue; // Skips '=' padding and whitespace.
		acc = (acc << 6) | (unsigned)d;
		bits += 6;
		if (bits >= 8) {
			bits -= 8;
			out[n++] = (uint8_t)(acc >> bits);
		}
	}
	*out_size = n;
	return out;
}

//--------------------------------------------------------------------------------------------------
// Accessor reads. Every accessor decodes into tightly-packed floats (or raw integers for
// joints/indices), converting component types and honoring bufferView strides.

typedef struct CM_Loader
{
	CM_Json json;
	int root;
	CM_LoadParams params;
	// One decoded blob per glTF buffer (GLB binary chunk, base64, or read callback).
	// Not owned unless flagged.
	uint8_t** buffers;      // ckit array.
	int* buffer_sizes;      // ckit array.
	int* buffer_owned;      // ckit array.
} CM_Loader;

// Percent-decodes a URI into a fresh ckit string ("my%20model.bin" -> "my model.bin").
static char* cm_uri_decode(const char* uri)
{
	char* s = NULL;
	for (const char* p = uri; *p; ++p) {
		if (*p == '%' && p[1] && p[2]) {
			int hi = p[1] >= 'a' ? p[1] - 'a' + 10 : p[1] >= 'A' ? p[1] - 'A' + 10 : p[1] - '0';
			int lo = p[2] >= 'a' ? p[2] - 'a' + 10 : p[2] >= 'A' ? p[2] - 'A' + 10 : p[2] - '0';
			apush(s, (char)((hi << 4) | lo));
			p += 2;
		} else {
			apush(s, *p);
		}
	}
	apush(s, '\0');
	return s;
}

// Resolves an external URI through the user's read callback, copying the result into a
// CM_ALLOC'd buffer so ownership is uniform. NULL when no callback or the read fails.
static uint8_t* cm_read_external(CM_Loader* l, const char* uri, int* out_size)
{
	if (!l->params.read_fn) return NULL;
	char* path = cm_uri_decode(uri);
	int size = 0;
	void* data = l->params.read_fn(path, &size, l->params.udata);
	afree(path);
	if (!data || size <= 0) return NULL;
	uint8_t* copy = (uint8_t*)CM_ALLOC((size_t)size);
	memcpy(copy, data, (size_t)size);
	if (l->params.free_fn) l->params.free_fn(data, l->params.udata);
	*out_size = size;
	return copy;
}

static int cm_component_size(int component_type)
{
	switch (component_type) {
	case 5120: case 5121: return 1; // i8, u8
	case 5122: case 5123: return 2; // i16, u16
	case 5125: case 5126: return 4; // u32, f32
	}
	return 0;
}

static int cm_type_components(const char* type)
{
	if (!strcmp(type, "SCALAR")) return 1;
	if (!strcmp(type, "VEC2")) return 2;
	if (!strcmp(type, "VEC3")) return 3;
	if (!strcmp(type, "VEC4")) return 4;
	if (!strcmp(type, "MAT4")) return 16;
	return 0;
}

// Resolves an accessor to (base pointer, stride, count, components, component type,
// normalized). Returns 0 on any out-of-bounds or unsupported layout.
typedef struct CM_Accessor
{
	const uint8_t* data;
	int stride;
	int count;
	int components;
	int component_type;
	int normalized;
} CM_Accessor;

static int cm_resolve_accessor(CM_Loader* l, int accessor_index, CM_Accessor* out)
{
	const CM_Json* j = &l->json;
	int accessors = cm_jget(j, l->root, "accessors");
	int a = cm_jat(j, accessors, accessor_index);
	if (a < 0) CM_FAIL("accessor %d missing", accessor_index);
	int component_type = cm_jget_int(j, a, "componentType", 0);
	int csize = cm_component_size(component_type);
	int components = cm_type_components(cm_jstr(j, cm_jget(j, a, "type"), ""));
	int count = cm_jget_int(j, a, "count", 0);
	if (!csize || !components || count <= 0) CM_FAIL("accessor %d has invalid layout", accessor_index);
	// A bufferView-backed accessor is bounded by its buffer below, but a zero-filled one
	// (sparse) has nothing bounding `count`, and callers allocate count * components
	// floats up front. No real asset holds 64M elements in one accessor.
	if (count > (1 << 26)) CM_FAIL("accessor %d has an implausible count (%d)", accessor_index, count);
	int bv = cm_jat(j, cm_jget(j, l->root, "bufferViews"), cm_jget_int(j, a, "bufferView", -1));
	if (bv < 0) {
		// No bufferView: a zero-filled base, legal for sparse accessors. cm_read_floats
		// handles it; direct binary readers (joints, indices) must check for NULL data.
		memset(out, 0, sizeof(*out));
		out->count = count;
		out->components = components;
		out->component_type = component_type;
		return 1;
	}
	int buffer = cm_jget_int(j, bv, "buffer", 0);
	if (buffer < 0 || buffer >= asize(l->buffers) || !l->buffers[buffer]) CM_FAIL("accessor %d references missing buffer data", accessor_index);
	int bv_offset = cm_jget_int(j, bv, "byteOffset", 0);
	int bv_length = cm_jget_int(j, bv, "byteLength", 0);
	int stride = cm_jget_int(j, bv, "byteStride", 0);
	if (!stride) stride = csize * components;
	int a_offset = cm_jget_int(j, a, "byteOffset", 0);
	// Offsets, strides, and counts are all file data. Negatives are nonsense, and the
	// span math must run in int64: `stride * (count - 1)` in int wraps on a hostile
	// stride, and a wrapped span passes any bounds check you write against it.
	if (bv_offset < 0 || bv_length < 0 || stride < 0 || a_offset < 0) CM_FAIL("accessor %d has a negative offset or stride", accessor_index);
	int64_t span = (int64_t)bv_offset + a_offset + (int64_t)stride * (count - 1) + (int64_t)csize * components;
	if (span > (int64_t)l->buffer_sizes[buffer] || (int64_t)bv_offset + bv_length > (int64_t)l->buffer_sizes[buffer]) {
		CM_FAIL("accessor %d reads out of bounds", accessor_index);
	}
	out->data = l->buffers[buffer] + bv_offset + a_offset;
	out->stride = stride;
	out->count = count;
	out->components = components;
	out->component_type = component_type;
	out->normalized = cm_jint(j, cm_jget(j, a, "normalized"), 0);
	return 1;
}

// Reads one component as a float, applying normalized-integer scaling per the spec.
static float cm_read_component(const uint8_t* p, int component_type, int normalized)
{
	switch (component_type) {
	case 5126: { float f; memcpy(&f, p, 4); return f; }
	case 5121: return normalized ? (float)(*p) / 255.0f : (float)(*p);
	case 5123: { uint16_t v; memcpy(&v, p, 2); return normalized ? (float)v / 65535.0f : (float)v; }
	case 5120: { int8_t v; memcpy(&v, p, 1); float f = (float)v; return normalized ? (f < -127.0f ? -1.0f : f / 127.0f) : f; }
	case 5122: { int16_t v; memcpy(&v, p, 2); float f = (float)v; return normalized ? (f < -32767.0f ? -1.0f : f / 32767.0f) : f; }
	case 5125: { uint32_t v; memcpy(&v, p, 4); return (float)v; }
	}
	return 0;
}

// Decodes an accessor into `want` floats per element (padding with 0, truncating extras),
// applying sparse substitution when present. Returns a CM_ALLOC'd array of count*want
// floats, or NULL.
static float* cm_read_floats(CM_Loader* l, int accessor_index, int want, int* out_count)
{
	const CM_Json* j = &l->json;
	CM_Accessor a;
	if (!cm_resolve_accessor(l, accessor_index, &a)) return NULL;
	int csize = cm_component_size(a.component_type);
	float* out = (float*)CM_ALLOC(sizeof(float) * (size_t)a.count * (size_t)want);
	if (a.data) {
		for (int i = 0; i < a.count; ++i) {
			const uint8_t* p = a.data + (size_t)i * (size_t)a.stride;
			for (int c = 0; c < want; ++c) {
				out[i * want + c] = c < a.components ? cm_read_component(p + c * csize, a.component_type, a.normalized) : 0.0f;
			}
		}
	} else {
		memset(out, 0, sizeof(float) * (size_t)a.count * (size_t)want);
	}

	// Sparse substitution: `count` overrides at explicit indices (morph targets live here).
	int ja = cm_jat(j, cm_jget(j, l->root, "accessors"), accessor_index);
	int sparse = cm_jget(j, ja, "sparse");
	if (sparse >= 0) {
		int jviews = cm_jget(j, l->root, "bufferViews");
		int sparse_count = cm_jget_int(j, sparse, "count", 0);
		int jindices = cm_jget(j, sparse, "indices");
		int jvalues = cm_jget(j, sparse, "values");
		int itype = cm_jget_int(j, jindices, "componentType", 0);
		int isize = cm_component_size(itype);
		int ibv = cm_jat(j, jviews, cm_jget_int(j, jindices, "bufferView", -1));
		int vbv = cm_jat(j, jviews, cm_jget_int(j, jvalues, "bufferView", -1));
		int ibuf = cm_jget_int(j, ibv, "buffer", 0);
		int vbuf = cm_jget_int(j, vbv, "buffer", 0);
		if (ibv < 0 || vbv < 0 || !isize || (itype != 5121 && itype != 5123 && itype != 5125)
			|| ibuf < 0 || ibuf >= asize(l->buffers) || !l->buffers[ibuf]
			|| vbuf < 0 || vbuf >= asize(l->buffers) || !l->buffers[vbuf]) {
			CM_FREE(out);
			snprintf(cm_g_error, sizeof(cm_g_error), "accessor %d has a malformed sparse block", accessor_index);
			return NULL;
		}
		int64_t ioffset = (int64_t)cm_jget_int(j, ibv, "byteOffset", 0) + cm_jget_int(j, jindices, "byteOffset", 0);
		int64_t voffset = (int64_t)cm_jget_int(j, vbv, "byteOffset", 0) + cm_jget_int(j, jvalues, "byteOffset", 0);
		int64_t vstride = (int64_t)csize * a.components;
		// Same discipline as cm_resolve_accessor: reject negatives, span in int64.
		if (sparse_count < 0 || ioffset < 0 || voffset < 0
			|| ioffset + (int64_t)isize * sparse_count > (int64_t)l->buffer_sizes[ibuf]
			|| voffset + vstride * sparse_count > (int64_t)l->buffer_sizes[vbuf]) {
			CM_FREE(out);
			snprintf(cm_g_error, sizeof(cm_g_error), "accessor %d sparse block reads out of bounds", accessor_index);
			return NULL;
		}
		const uint8_t* ip = l->buffers[ibuf] + ioffset;
		const uint8_t* vp = l->buffers[vbuf] + voffset;
		for (int s = 0; s < sparse_count; ++s) {
			uint32_t index;
			if (isize == 1) index = ip[s];
			else if (isize == 2) { uint16_t v; memcpy(&v, ip + s * 2, 2); index = v; }
			else { memcpy(&index, ip + s * 4, 4); }
			if (index >= (uint32_t)a.count) continue;
			const uint8_t* p = vp + (size_t)s * (size_t)vstride;
			for (int c = 0; c < want; ++c) {
				out[index * want + c] = c < a.components ? cm_read_component(p + c * csize, a.component_type, a.normalized) : 0.0f;
			}
		}
	}
	*out_count = a.count;
	return out;
}

//--------------------------------------------------------------------------------------------------
// Small column-major mat4 + quaternion kit (just enough for posing).

static void cm_mat4_identity(float* m)
{
	memset(m, 0, sizeof(float) * 16);
	m[0] = m[5] = m[10] = m[15] = 1;
}

static void cm_mat4_mul(const float* a, const float* b, float* out) // out = a * b
{
	float r[16];
	for (int c = 0; c < 4; ++c) {
		for (int rr = 0; rr < 4; ++rr) {
			r[c * 4 + rr] = a[0 * 4 + rr] * b[c * 4 + 0]
			              + a[1 * 4 + rr] * b[c * 4 + 1]
			              + a[2 * 4 + rr] * b[c * 4 + 2]
			              + a[3 * 4 + rr] * b[c * 4 + 3];
		}
	}
	memcpy(out, r, sizeof(r));
}

static void cm_trs_to_mat4(const CM_Transform* t, float* m)
{
	float x = t->rotation[0], y = t->rotation[1], z = t->rotation[2], w = t->rotation[3];
	float x2 = x + x, y2 = y + y, z2 = z + z;
	float xx = x * x2, xy = x * y2, xz = x * z2;
	float yy = y * y2, yz = y * z2, zz = z * z2;
	float wx = w * x2, wy = w * y2, wz = w * z2;
	float sx = t->scale[0], sy = t->scale[1], sz = t->scale[2];
	m[0] = (1 - (yy + zz)) * sx; m[1] = (xy + wz) * sx;       m[2] = (xz - wy) * sx;       m[3] = 0;
	m[4] = (xy - wz) * sy;       m[5] = (1 - (xx + zz)) * sy; m[6] = (yz + wx) * sy;       m[7] = 0;
	m[8] = (xz + wy) * sz;       m[9] = (yz - wx) * sz;       m[10] = (1 - (xx + yy)) * sz; m[11] = 0;
	m[12] = t->translation[0];   m[13] = t->translation[1];   m[14] = t->translation[2];   m[15] = 1;
}

// Decomposes an affine matrix into TRS, assuming no shear (scale = column lengths).
static void cm_mat4_decompose(const float* m, CM_Transform* out)
{
	out->translation[0] = m[12]; out->translation[1] = m[13]; out->translation[2] = m[14];
	float sx = sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
	float sy = sqrtf(m[4] * m[4] + m[5] * m[5] + m[6] * m[6]);
	float sz = sqrtf(m[8] * m[8] + m[9] * m[9] + m[10] * m[10]);
	// Negative determinant means one axis is mirrored; fold the flip into x.
	float det = m[0] * (m[5] * m[10] - m[6] * m[9]) - m[4] * (m[1] * m[10] - m[2] * m[9]) + m[8] * (m[1] * m[6] - m[2] * m[5]);
	if (det < 0) sx = -sx;
	out->scale[0] = sx; out->scale[1] = sy; out->scale[2] = sz;
	float r[9] = {
		m[0] / (sx ? sx : 1), m[1] / (sx ? sx : 1), m[2] / (sx ? sx : 1),
		m[4] / (sy ? sy : 1), m[5] / (sy ? sy : 1), m[6] / (sy ? sy : 1),
		m[8] / (sz ? sz : 1), m[9] / (sz ? sz : 1), m[10] / (sz ? sz : 1),
	};
	float trace = r[0] + r[4] + r[8];
	float* q = out->rotation;
	if (trace > 0) {
		float s = sqrtf(trace + 1.0f) * 2;
		q[3] = 0.25f * s; q[0] = (r[5] - r[7]) / s; q[1] = (r[6] - r[2]) / s; q[2] = (r[1] - r[3]) / s;
	} else if (r[0] > r[4] && r[0] > r[8]) {
		float s = sqrtf(1.0f + r[0] - r[4] - r[8]) * 2;
		q[3] = (r[5] - r[7]) / s; q[0] = 0.25f * s; q[1] = (r[3] + r[1]) / s; q[2] = (r[6] + r[2]) / s;
	} else if (r[4] > r[8]) {
		float s = sqrtf(1.0f + r[4] - r[0] - r[8]) * 2;
		q[3] = (r[6] - r[2]) / s; q[0] = (r[3] + r[1]) / s; q[1] = 0.25f * s; q[2] = (r[7] + r[5]) / s;
	} else {
		float s = sqrtf(1.0f + r[8] - r[0] - r[4]) * 2;
		q[3] = (r[1] - r[3]) / s; q[0] = (r[6] + r[2]) / s; q[1] = (r[7] + r[5]) / s; q[2] = 0.25f * s;
	}
}

static void cm_quat_slerp(const float* a, const float* b, float t, float* out)
{
	// Shortest path; falls back to nlerp when nearly parallel.
	float bx = b[0], by = b[1], bz = b[2], bw = b[3];
	float dot = a[0] * bx + a[1] * by + a[2] * bz + a[3] * bw;
	if (dot < 0) { bx = -bx; by = -by; bz = -bz; bw = -bw; dot = -dot; }
	float wa, wb;
	if (dot > 0.9995f) {
		wa = 1.0f - t;
		wb = t;
	} else {
		float theta = acosf(dot);
		float s = sinf(theta);
		wa = sinf((1.0f - t) * theta) / s;
		wb = sinf(t * theta) / s;
	}
	float x = a[0] * wa + bx * wb;
	float y = a[1] * wa + by * wb;
	float z = a[2] * wa + bz * wb;
	float w = a[3] * wa + bw * wb;
	float len = sqrtf(x * x + y * y + z * z + w * w);
	if (len == 0) { out[0] = out[1] = out[2] = 0; out[3] = 1; return; }
	out[0] = x / len; out[1] = y / len; out[2] = z / len; out[3] = w / len;
}

//--------------------------------------------------------------------------------------------------
// Model building.

static void cm_default_transform(CM_Transform* t)
{
	memset(t, 0, sizeof(*t));
	t->rotation[3] = 1;
	t->scale[0] = t->scale[1] = t->scale[2] = 1;
}

// Smooth normals: accumulate area-weighted face normals, then normalize.
static float* cm_generate_normals(const float* positions, int vertex_count, const uint32_t* indices, int index_count)
{
	float* n = (float*)CM_ALLOC(sizeof(float) * 3 * (size_t)vertex_count);
	memset(n, 0, sizeof(float) * 3 * (size_t)vertex_count);
	for (int i = 0; i + 2 < index_count; i += 3) {
		uint32_t ia = indices[i], ib = indices[i + 1], ic = indices[i + 2];
		if (ia >= (uint32_t)vertex_count || ib >= (uint32_t)vertex_count || ic >= (uint32_t)vertex_count) continue;
		const float* a = positions + ia * 3;
		const float* b = positions + ib * 3;
		const float* c = positions + ic * 3;
		float e1[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
		float e2[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };
		float fn[3] = { e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0] };
		for (int k = 0; k < 3; ++k) {
			n[ia * 3 + k] += fn[k];
			n[ib * 3 + k] += fn[k];
			n[ic * 3 + k] += fn[k];
		}
	}
	for (int i = 0; i < vertex_count; ++i) {
		float* v = n + i * 3;
		float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		if (len > 0) { v[0] /= len; v[1] /= len; v[2] /= len; }
		else v[1] = 1;
	}
	return n;
}

// Area-weighted tangent accumulation (Lengyel), orthonormalized against the normal;
// w carries the bitangent's handedness sign. Bake-critical normal maps should ship
// authored (MikkTSpace) tangents instead.
static float* cm_generate_tangents(const float* positions, const float* normals, const float* uvs, int vertex_count, const uint32_t* indices, int index_count)
{
	float* tan = (float*)CM_ALLOC(sizeof(float) * 3 * (size_t)vertex_count);
	float* bitan = (float*)CM_ALLOC(sizeof(float) * 3 * (size_t)vertex_count);
	memset(tan, 0, sizeof(float) * 3 * (size_t)vertex_count);
	memset(bitan, 0, sizeof(float) * 3 * (size_t)vertex_count);
	for (int i = 0; i + 2 < index_count; i += 3) {
		uint32_t ia = indices[i], ib = indices[i + 1], ic = indices[i + 2];
		if (ia >= (uint32_t)vertex_count || ib >= (uint32_t)vertex_count || ic >= (uint32_t)vertex_count) continue;
		const float* pa = positions + ia * 3;
		const float* pb = positions + ib * 3;
		const float* pc = positions + ic * 3;
		float e1[3] = { pb[0] - pa[0], pb[1] - pa[1], pb[2] - pa[2] };
		float e2[3] = { pc[0] - pa[0], pc[1] - pa[1], pc[2] - pa[2] };
		float du1 = uvs[ib * 2] - uvs[ia * 2], dv1 = uvs[ib * 2 + 1] - uvs[ia * 2 + 1];
		float du2 = uvs[ic * 2] - uvs[ia * 2], dv2 = uvs[ic * 2 + 1] - uvs[ia * 2 + 1];
		float det = du1 * dv2 - du2 * dv1;
		if (det == 0) continue;
		float r = 1.0f / det;
		float t[3] = { (e1[0] * dv2 - e2[0] * dv1) * r, (e1[1] * dv2 - e2[1] * dv1) * r, (e1[2] * dv2 - e2[2] * dv1) * r };
		float b[3] = { (e2[0] * du1 - e1[0] * du2) * r, (e2[1] * du1 - e1[1] * du2) * r, (e2[2] * du1 - e1[2] * du2) * r };
		for (int k = 0; k < 3; ++k) {
			tan[ia * 3 + k] += t[k]; tan[ib * 3 + k] += t[k]; tan[ic * 3 + k] += t[k];
			bitan[ia * 3 + k] += b[k]; bitan[ib * 3 + k] += b[k]; bitan[ic * 3 + k] += b[k];
		}
	}
	float* out = (float*)CM_ALLOC(sizeof(float) * 4 * (size_t)vertex_count);
	for (int i = 0; i < vertex_count; ++i) {
		const float* n = normals + i * 3;
		const float* t = tan + i * 3;
		// Gram-Schmidt against the normal.
		float ndt = n[0] * t[0] + n[1] * t[1] + n[2] * t[2];
		float o[3] = { t[0] - n[0] * ndt, t[1] - n[1] * ndt, t[2] - n[2] * ndt };
		float len = sqrtf(o[0] * o[0] + o[1] * o[1] + o[2] * o[2]);
		if (len > 1.0e-8f) { o[0] /= len; o[1] /= len; o[2] /= len; }
		else { o[0] = 1; o[1] = 0; o[2] = 0; } // Degenerate uv area: any perpendicular works.
		float cx = n[1] * t[2] - n[2] * t[1];
		float cy = n[2] * t[0] - n[0] * t[2];
		float cz = n[0] * t[1] - n[1] * t[0];
		const float* b = bitan + i * 3;
		float w = (cx * b[0] + cy * b[1] + cz * b[2]) < 0 ? -1.0f : 1.0f;
		out[i * 4] = o[0]; out[i * 4 + 1] = o[1]; out[i * 4 + 2] = o[2]; out[i * 4 + 3] = w;
	}
	CM_FREE(tan);
	CM_FREE(bitan);
	return out;
}

static int cm_load_primitive(CM_Loader* l, int prim, CM_Primitive* out)
{
	const CM_Json* j = &l->json;
	memset(out, 0, sizeof(*out));
	out->material = cm_jget_int(j, prim, "material", -1);
	if (cm_jget_int(j, prim, "mode", 4) != 4) CM_FAIL("only triangle primitives are supported");
	int attrs = cm_jget(j, prim, "attributes");
	int pos_accessor = cm_jget_int(j, attrs, "POSITION", -1);
	if (pos_accessor < 0) CM_FAIL("primitive has no POSITION attribute");
	out->positions = cm_read_floats(l, pos_accessor, 3, &out->vertex_count);
	if (!out->positions) return 0;

	// Position bounds, computed from the stream rather than trusted from the accessor's
	// min/max json (files can lie; the loaded floats cannot). One pass over data already
	// hot in cache -- this is what culling and auto-framing want from every model.
	if (out->vertex_count > 0) {
		for (int c = 0; c < 3; ++c) {
			out->aabb_min[c] = out->positions[c];
			out->aabb_max[c] = out->positions[c];
		}
		for (int i = 1; i < out->vertex_count; ++i) {
			for (int c = 0; c < 3; ++c) {
				float v = out->positions[i * 3 + c];
				if (v < out->aabb_min[c]) out->aabb_min[c] = v;
				if (v > out->aabb_max[c]) out->aabb_max[c] = v;
			}
		}
	}

	// Per spec every attribute accessor in a primitive has the same count. Enforcing it
	// is what lets the generators and the interleaving loop below index all streams with
	// one vertex index; a short stream would be read out of bounds.
	#define CM_CHECK_COUNT(n, what) do { \
		if ((n) < out->vertex_count) CM_FAIL("primitive attribute %s has %d elements, POSITION has %d", (what), (n), out->vertex_count); \
	} while (0)

	int uv_accessor = cm_jget_int(j, attrs, "TEXCOORD_0", -1);
	if (uv_accessor >= 0) {
		int n;
		out->uvs = cm_read_floats(l, uv_accessor, 2, &n);
		if (!out->uvs) return 0;
		CM_CHECK_COUNT(n, "TEXCOORD_0");
	}

	int uv1_accessor = cm_jget_int(j, attrs, "TEXCOORD_1", -1);
	if (uv1_accessor >= 0) {
		int n;
		out->uvs1 = cm_read_floats(l, uv1_accessor, 2, &n);
		if (!out->uvs1) return 0;
		CM_CHECK_COUNT(n, "TEXCOORD_1");
	}

	int color_accessor = cm_jget_int(j, attrs, "COLOR_0", -1);
	if (color_accessor >= 0) {
		// COLOR_0 may be VEC3; cm_read_floats pads with 0, so force alpha to 1 after.
		CM_Accessor ca;
		if (!cm_resolve_accessor(l, color_accessor, &ca)) return 0;
		int n;
		out->colors = cm_read_floats(l, color_accessor, 4, &n);
		if (!out->colors) return 0;
		CM_CHECK_COUNT(n, "COLOR_0");
		if (ca.components == 3) {
			for (int i = 0; i < n; ++i) out->colors[i * 4 + 3] = 1.0f;
		}
	}

	int joints_accessor = cm_jget_int(j, attrs, "JOINTS_0", -1);
	int weights_accessor = cm_jget_int(j, attrs, "WEIGHTS_0", -1);
	if (joints_accessor >= 0 && weights_accessor >= 0) {
		CM_Accessor ja;
		if (!cm_resolve_accessor(l, joints_accessor, &ja)) return 0;
		int csize = cm_component_size(ja.component_type);
		if (!ja.data) CM_FAIL("sparse JOINTS_0 is not supported");
		if (ja.component_type != 5121 && ja.component_type != 5123) CM_FAIL("JOINTS_0 must be u8 or u16");
		CM_CHECK_COUNT(ja.count, "JOINTS_0");
		out->joints = (uint16_t*)CM_ALLOC(sizeof(uint16_t) * 4 * (size_t)ja.count);
		for (int i = 0; i < ja.count; ++i) {
			const uint8_t* p = ja.data + (size_t)i * (size_t)ja.stride;
			for (int c = 0; c < 4; ++c) {
				if (c >= ja.components) { out->joints[i * 4 + c] = 0; continue; }
				if (csize == 1) out->joints[i * 4 + c] = p[c];
				else { uint16_t v; memcpy(&v, p + c * 2, 2); out->joints[i * 4 + c] = v; }
			}
		}
		int n;
		out->weights = cm_read_floats(l, weights_accessor, 4, &n);
		if (!out->weights) return 0;
		CM_CHECK_COUNT(n, "WEIGHTS_0");
	}

	int indices_accessor = cm_jget_int(j, prim, "indices", -1);
	if (indices_accessor >= 0) {
		CM_Accessor ia;
		if (!cm_resolve_accessor(l, indices_accessor, &ia)) return 0;
		int csize = cm_component_size(ia.component_type);
		if (!ia.data) CM_FAIL("sparse indices are not supported");
		if (ia.component_type != 5121 && ia.component_type != 5123 && ia.component_type != 5125) CM_FAIL("indices must be u8, u16, or u32");
		out->index_count = ia.count;
		out->indices = (uint32_t*)CM_ALLOC(sizeof(uint32_t) * (size_t)ia.count);
		for (int i = 0; i < ia.count; ++i) {
			const uint8_t* p = ia.data + (size_t)i * (size_t)ia.stride;
			if (csize == 1) out->indices[i] = p[0];
			else if (csize == 2) { uint16_t v; memcpy(&v, p, 2); out->indices[i] = v; }
			else { uint32_t v; memcpy(&v, p, 4); out->indices[i] = v; }
		}
	} else {
		out->index_count = out->vertex_count;
		out->indices = (uint32_t*)CM_ALLOC(sizeof(uint32_t) * (size_t)out->vertex_count);
		for (int i = 0; i < out->vertex_count; ++i) out->indices[i] = (uint32_t)i;
	}

	int normal_accessor = cm_jget_int(j, attrs, "NORMAL", -1);
	if (normal_accessor >= 0) {
		int n;
		out->normals = cm_read_floats(l, normal_accessor, 3, &n);
		if (!out->normals) return 0;
		CM_CHECK_COUNT(n, "NORMAL");
	} else {
		out->normals = cm_generate_normals(out->positions, out->vertex_count, out->indices, out->index_count);
	}

	int tangent_accessor = cm_jget_int(j, attrs, "TANGENT", -1);
	if (tangent_accessor >= 0) {
		int n;
		out->tangents = cm_read_floats(l, tangent_accessor, 4, &n);
		if (!out->tangents) return 0;
		CM_CHECK_COUNT(n, "TANGENT");
	} else if (out->uvs) {
		out->tangents = cm_generate_tangents(out->positions, out->normals, out->uvs, out->vertex_count, out->indices, out->index_count);
	}

	// Morph targets: per-target delta streams (sparse accessors are the common encoding).
	int jtargets = cm_jget(j, prim, "targets");
	out->target_count = cm_jlen(j, jtargets);
	if (out->target_count) {
		out->targets = (CM_MorphTarget*)CM_ALLOC(sizeof(CM_MorphTarget) * (size_t)out->target_count);
		memset(out->targets, 0, sizeof(CM_MorphTarget) * (size_t)out->target_count);
		for (int t = 0; t < out->target_count; ++t) {
			int target = cm_jat(j, jtargets, t);
			int n;
			int pos = cm_jget_int(j, target, "POSITION", -1);
			int nrm = cm_jget_int(j, target, "NORMAL", -1);
			int tan = cm_jget_int(j, target, "TANGENT", -1);
			if (pos >= 0) {
				if (!(out->targets[t].positions = cm_read_floats(l, pos, 3, &n))) return 0;
				CM_CHECK_COUNT(n, "morph target POSITION");
			}
			if (nrm >= 0) {
				if (!(out->targets[t].normals = cm_read_floats(l, nrm, 3, &n))) return 0;
				CM_CHECK_COUNT(n, "morph target NORMAL");
			}
			if (tan >= 0) {
				if (!(out->targets[t].tangents = cm_read_floats(l, tan, 3, &n))) return 0;
				CM_CHECK_COUNT(n, "morph target TANGENT");
			}
		}
	}
	#undef CM_CHECK_COUNT
	return 1;
}

static void cm_free_primitive(CM_Primitive* p)
{
	if (p->positions) CM_FREE(p->positions);
	if (p->normals) CM_FREE(p->normals);
	if (p->tangents) CM_FREE(p->tangents);
	if (p->uvs) CM_FREE(p->uvs);
	if (p->uvs1) CM_FREE(p->uvs1);
	if (p->colors) CM_FREE(p->colors);
	if (p->joints) CM_FREE(p->joints);
	if (p->weights) CM_FREE(p->weights);
	if (p->indices) CM_FREE(p->indices);
	for (int t = 0; t < p->target_count; ++t) {
		if (p->targets[t].positions) CM_FREE(p->targets[t].positions);
		if (p->targets[t].normals) CM_FREE(p->targets[t].normals);
		if (p->targets[t].tangents) CM_FREE(p->targets[t].tangents);
	}
	if (p->targets) CM_FREE(p->targets);
}

// Copies an object's raw "extras" JSON out of the source text, or NULL when absent.
static const char* cm_extras(CM_Loader* l, int obj)
{
	int e = cm_jget(&l->json, obj, "extras");
	if (e < 0) return NULL;
	const CM_JVal* v = l->json.vals + e;
	int n = (int)(v->src_end - v->src_begin);
	char* s = (char*)CM_ALLOC((size_t)n + 1);
	memcpy(s, v->src_begin, (size_t)n);
	s[n] = 0;
	return s;
}

void cm_free(CM_Model* model)
{
	if (!model) return;
	for (int i = 0; i < model->mesh_count; ++i) {
		for (int p = 0; p < model->meshes[i].primitive_count; ++p) cm_free_primitive(model->meshes[i].primitives + p);
		if (model->meshes[i].primitives) CM_FREE(model->meshes[i].primitives);
		if (model->meshes[i].weights) CM_FREE(model->meshes[i].weights);
		if (model->meshes[i].extras) CM_FREE((void*)model->meshes[i].extras);
	}
	if (model->meshes) CM_FREE(model->meshes);
	for (int i = 0; i < model->node_count; ++i) {
		if (model->nodes[i].children) CM_FREE(model->nodes[i].children);
		if (model->nodes[i].extras) CM_FREE((void*)model->nodes[i].extras);
	}
	if (model->nodes) CM_FREE(model->nodes);
	for (int i = 0; i < model->material_count; ++i) {
		if (model->materials[i].extras) CM_FREE((void*)model->materials[i].extras);
	}
	if (model->lights) CM_FREE(model->lights);
	for (int i = 0; i < model->skin_count; ++i) {
		if (model->skins[i].joints) CM_FREE(model->skins[i].joints);
		if (model->skins[i].inverse_binds) CM_FREE(model->skins[i].inverse_binds);
	}
	if (model->skins) CM_FREE(model->skins);
	for (int i = 0; i < model->animation_count; ++i) {
		for (int c = 0; c < model->animations[i].channel_count; ++c) {
			if (model->animations[i].channels[c].times) CM_FREE(model->animations[i].channels[c].times);
			if (model->animations[i].channels[c].values) CM_FREE(model->animations[i].channels[c].values);
		}
		if (model->animations[i].channels) CM_FREE(model->animations[i].channels);
	}
	if (model->animations) CM_FREE(model->animations);
	for (int i = 0; i < model->image_count; ++i) {
		if (model->images[i].data) CM_FREE(model->images[i].data);
	}
	if (model->images) CM_FREE(model->images);
	if (model->materials) CM_FREE(model->materials);
	if (model->order) CM_FREE(model->order);
	CM_FREE(model);
}

// Resolves a glTF textureInfo object to an image index plus its KHR_texture_transform.
static CM_TextureRef cm_load_texture_ref(CM_Loader* l, int tex_info)
{
	const CM_Json* j = &l->json;
	CM_TextureRef ref;
	memset(&ref, 0, sizeof(ref));
	ref.image = -1;
	ref.scale[0] = ref.scale[1] = 1;
	int texture = cm_jat(j, cm_jget(j, l->root, "textures"), cm_jget_int(j, tex_info, "index", -1));
	if (texture < 0) return ref;
	ref.image = cm_jget_int(j, texture, "source", -1);
	ref.uv_set = cm_jget_int(j, tex_info, "texCoord", 0);
	int xform = cm_jget(j, cm_jget(j, tex_info, "extensions"), "KHR_texture_transform");
	if (xform >= 0) {
		int off = cm_jget(j, xform, "offset");
		int sc = cm_jget(j, xform, "scale");
		for (int k = 0; k < 2 && k < cm_jlen(j, off); ++k) ref.offset[k] = (float)cm_jnum(j, cm_jat(j, off, k), 0);
		for (int k = 0; k < 2 && k < cm_jlen(j, sc); ++k) ref.scale[k] = (float)cm_jnum(j, cm_jat(j, sc, k), 1);
		ref.rotation = (float)cm_jnum(j, cm_jget(j, xform, "rotation"), 0);
	}
	return ref;
}

static CM_Model* cm_load_from_json(CM_Loader* l)
{
	const CM_Json* j = &l->json;
	CM_Model* model = (CM_Model*)CM_ALLOC(sizeof(CM_Model));
	memset(model, 0, sizeof(*model));

	// Nodes + hierarchy.
	int jnodes = cm_jget(j, l->root, "nodes");
	model->node_count = cm_jlen(j, jnodes);
	if (model->node_count) {
		model->nodes = (CM_Node*)CM_ALLOC(sizeof(CM_Node) * (size_t)model->node_count);
		memset(model->nodes, 0, sizeof(CM_Node) * (size_t)model->node_count);
	}
	for (int i = 0; i < model->node_count; ++i) {
		int n = cm_jat(j, jnodes, i);
		CM_Node* node = model->nodes + i;
		node->name = cm_jstr(j, cm_jget(j, n, "name"), "");
		node->parent = -1;
		node->mesh = cm_jget_int(j, n, "mesh", -1);
		node->skin = cm_jget_int(j, n, "skin", -1);
		node->light = cm_jget_int(j, cm_jget(j, cm_jget(j, n, "extensions"), "KHR_lights_punctual"), "light", -1);
		node->extras = cm_extras(l, n);
		cm_default_transform(&node->rest);
		int matrix = cm_jget(j, n, "matrix");
		if (matrix >= 0 && cm_jlen(j, matrix) == 16) {
			float m[16];
			for (int k = 0; k < 16; ++k) m[k] = (float)cm_jnum(j, cm_jat(j, matrix, k), 0);
			cm_mat4_decompose(m, &node->rest);
		} else {
			int t = cm_jget(j, n, "translation");
			int r = cm_jget(j, n, "rotation");
			int s = cm_jget(j, n, "scale");
			for (int k = 0; k < 3 && k < cm_jlen(j, t); ++k) node->rest.translation[k] = (float)cm_jnum(j, cm_jat(j, t, k), 0);
			for (int k = 0; k < 4 && k < cm_jlen(j, r); ++k) node->rest.rotation[k] = (float)cm_jnum(j, cm_jat(j, r, k), 0);
			for (int k = 0; k < 3 && k < cm_jlen(j, s); ++k) node->rest.scale[k] = (float)cm_jnum(j, cm_jat(j, s, k), 1);
		}
		// Child indices are file data: drop anything out of range (self-references
		// included) so the hierarchy walk below can trust them.
		int children = cm_jget(j, n, "children");
		int listed = cm_jlen(j, children);
		if (listed) {
			node->children = (int*)CM_ALLOC(sizeof(int) * (size_t)listed);
			for (int k = 0; k < listed; ++k) {
				int c = cm_jint(j, cm_jat(j, children, k), -1);
				if (c >= 0 && c < model->node_count && c != i) node->children[node->child_count++] = c;
			}
		}
	}
	// Parent links, first writer wins: a node listed under two parents is not a tree, and
	// the second link would orphan it from the traversal below.
	for (int i = 0; i < model->node_count; ++i) {
		for (int k = 0; k < model->nodes[i].child_count; ++k) {
			int c = model->nodes[i].children[k];
			if (model->nodes[c].parent < 0) model->nodes[c].parent = i;
		}
	}

	// Topological order: roots first, then children breadth-first. `emitted` keeps a
	// malformed hierarchy (a node reachable from several parents, or a cycle) from
	// pushing the same node twice and running off the end of the array.
	model->order = (int*)CM_ALLOC(sizeof(int) * (size_t)(model->node_count ? model->node_count : 1));
	{
		uint8_t* emitted = (uint8_t*)CM_ALLOC((size_t)(model->node_count ? model->node_count : 1));
		memset(emitted, 0, (size_t)(model->node_count ? model->node_count : 1));
		int n = 0;
		for (int i = 0; i < model->node_count; ++i) {
			if (model->nodes[i].parent < 0) { emitted[i] = 1; model->order[n++] = i; }
		}
		for (int cursor = 0; cursor < n; ++cursor) {
			CM_Node* node = model->nodes + model->order[cursor];
			for (int k = 0; k < node->child_count; ++k) {
				int c = node->children[k];
				if (emitted[c]) continue;
				emitted[c] = 1;
				model->order[n++] = c;
			}
		}
		int complete = n == model->node_count;
		CM_FREE(emitted);
		if (!complete) { cm_free(model); CM_FAIL("node hierarchy contains a cycle"); }
	}

	// Meshes.
	int jmeshes = cm_jget(j, l->root, "meshes");
	model->mesh_count = cm_jlen(j, jmeshes);
	if (model->mesh_count) {
		model->meshes = (CM_Mesh*)CM_ALLOC(sizeof(CM_Mesh) * (size_t)model->mesh_count);
		memset(model->meshes, 0, sizeof(CM_Mesh) * (size_t)model->mesh_count);
	}
	for (int i = 0; i < model->mesh_count; ++i) {
		int m = cm_jat(j, jmeshes, i);
		CM_Mesh* mesh = model->meshes + i;
		mesh->name = cm_jstr(j, cm_jget(j, m, "name"), "");
		mesh->extras = cm_extras(l, m);
		int jweights = cm_jget(j, m, "weights");
		mesh->weight_count = cm_jlen(j, jweights);
		if (mesh->weight_count) {
			mesh->weights = (float*)CM_ALLOC(sizeof(float) * (size_t)mesh->weight_count);
			for (int k = 0; k < mesh->weight_count; ++k) mesh->weights[k] = (float)cm_jnum(j, cm_jat(j, jweights, k), 0);
		}
		int prims = cm_jget(j, m, "primitives");
		mesh->primitive_count = cm_jlen(j, prims);
		if (mesh->primitive_count) {
			mesh->primitives = (CM_Primitive*)CM_ALLOC(sizeof(CM_Primitive) * (size_t)mesh->primitive_count);
			memset(mesh->primitives, 0, sizeof(CM_Primitive) * (size_t)mesh->primitive_count);
		}
		for (int p = 0; p < mesh->primitive_count; ++p) {
			if (!cm_load_primitive(l, cm_jat(j, prims, p), mesh->primitives + p)) {
				cm_free(model);
				return NULL;
			}
		}
	}

	// Skins.
	int jskins = cm_jget(j, l->root, "skins");
	model->skin_count = cm_jlen(j, jskins);
	if (model->skin_count) {
		model->skins = (CM_Skin*)CM_ALLOC(sizeof(CM_Skin) * (size_t)model->skin_count);
		memset(model->skins, 0, sizeof(CM_Skin) * (size_t)model->skin_count);
	}
	for (int i = 0; i < model->skin_count; ++i) {
		int s = cm_jat(j, jskins, i);
		CM_Skin* skin = model->skins + i;
		skin->name = cm_jstr(j, cm_jget(j, s, "name"), "");
		int joints = cm_jget(j, s, "joints");
		skin->joint_count = cm_jlen(j, joints);
		if (!skin->joint_count) { cm_free(model); CM_FAIL("skin %d has no joints", i); }
		skin->joints = (int*)CM_ALLOC(sizeof(int) * (size_t)skin->joint_count);
		for (int k = 0; k < skin->joint_count; ++k) skin->joints[k] = cm_jint(j, cm_jat(j, joints, k), 0);
		int ibm = cm_jget_int(j, s, "inverseBindMatrices", -1);
		if (ibm >= 0) {
			int n;
			skin->inverse_binds = cm_read_floats(l, ibm, 16, &n);
			if (!skin->inverse_binds || n < skin->joint_count) { cm_free(model); return NULL; }
		} else {
			skin->inverse_binds = (float*)CM_ALLOC(sizeof(float) * 16 * (size_t)skin->joint_count);
			for (int k = 0; k < skin->joint_count; ++k) cm_mat4_identity(skin->inverse_binds + k * 16);
		}
	}

	// Animations.
	int janims = cm_jget(j, l->root, "animations");
	model->animation_count = cm_jlen(j, janims);
	if (model->animation_count) {
		model->animations = (CM_Animation*)CM_ALLOC(sizeof(CM_Animation) * (size_t)model->animation_count);
		memset(model->animations, 0, sizeof(CM_Animation) * (size_t)model->animation_count);
	}
	for (int i = 0; i < model->animation_count; ++i) {
		int a = cm_jat(j, janims, i);
		CM_Animation* anim = model->animations + i;
		anim->name = cm_jstr(j, cm_jget(j, a, "name"), "");
		int channels = cm_jget(j, a, "channels");
		int samplers = cm_jget(j, a, "samplers");
		int channel_count = cm_jlen(j, channels);
		if (channel_count) {
			anim->channels = (CM_Channel*)CM_ALLOC(sizeof(CM_Channel) * (size_t)channel_count);
			memset(anim->channels, 0, sizeof(CM_Channel) * (size_t)channel_count);
		}
		for (int c = 0; c < channel_count; ++c) {
			int ch = cm_jat(j, channels, c);
			int target = cm_jget(j, ch, "target");
			const char* path = cm_jstr(j, cm_jget(j, target, "path"), "");
			int node = cm_jget_int(j, target, "node", -1);
			if (node < 0 || node >= model->node_count) continue;
			CM_ChannelPath cp;
			int lanes;
			if (!strcmp(path, "translation")) { cp = CM_PATH_TRANSLATION; lanes = 3; }
			else if (!strcmp(path, "rotation")) { cp = CM_PATH_ROTATION; lanes = 4; }
			else if (!strcmp(path, "scale")) { cp = CM_PATH_SCALE; lanes = 3; }
			else if (!strcmp(path, "weights")) {
				// Morph weights: one lane per target on the node's mesh.
				cp = CM_PATH_WEIGHTS;
				int mesh = model->nodes[node].mesh;
				lanes = mesh >= 0 && mesh < model->mesh_count && model->meshes[mesh].primitive_count
					? model->meshes[mesh].primitives[0].target_count : 0;
				if (!lanes) continue;
			}
			else continue;
			int sampler = cm_jat(j, samplers, cm_jget_int(j, ch, "sampler", -1));
			if (sampler < 0) continue;
			const char* interp = cm_jstr(j, cm_jget(j, sampler, "interpolation"), "LINEAR");
			int cubic = !strcmp(interp, "CUBICSPLINE");
			CM_Channel* out = anim->channels + anim->channel_count;
			out->node = node;
			out->path = cp;
			out->component_count = lanes;
			out->interpolation = cubic ? CM_INTERP_CUBIC : !strcmp(interp, "STEP") ? CM_INTERP_STEP : CM_INTERP_LINEAR;
			int key_count = 0;
			out->times = cm_read_floats(l, cm_jget_int(j, sampler, "input", -1), 1, &key_count);
			if (!out->times) { cm_free(model); return NULL; }
			out->key_count = key_count;
			// Weights outputs are SCALAR runs (lanes per key); vector paths read whole
			// elements. Both land in the same key-major float layout.
			int value_count = 0;
			float* values;
			if (cp == CM_PATH_WEIGHTS) {
				values = cm_read_floats(l, cm_jget_int(j, sampler, "output", -1), 1, &value_count);
				value_count /= lanes;
			} else {
				values = cm_read_floats(l, cm_jget_int(j, sampler, "output", -1), lanes, &value_count);
			}
			if (!values) { CM_FREE(out->times); out->times = NULL; cm_free(model); return NULL; }
			// CUBICSPLINE keys are in-tangent/value/out-tangent triples, kept verbatim for
			// Hermite evaluation at sample time.
			if (value_count < (cubic ? key_count * 3 : key_count)) {
				CM_FREE(values); CM_FREE(out->times); out->times = NULL; cm_free(model);
				CM_FAIL("animation channel is short on values");
			}
			out->values = values;
			if (key_count && out->times[key_count - 1] > anim->duration) anim->duration = out->times[key_count - 1];
			anim->channel_count++;
		}
	}

	// Images (left encoded; decode with your image library).
	int jimages = cm_jget(j, l->root, "images");
	model->image_count = cm_jlen(j, jimages);
	if (model->image_count) {
		model->images = (CM_Image*)CM_ALLOC(sizeof(CM_Image) * (size_t)model->image_count);
		memset(model->images, 0, sizeof(CM_Image) * (size_t)model->image_count);
	}
	for (int i = 0; i < model->image_count; ++i) {
		int im = cm_jat(j, jimages, i);
		CM_Image* image = model->images + i;
		image->name = cm_jstr(j, cm_jget(j, im, "name"), "");
		image->mime_type = cm_jstr(j, cm_jget(j, im, "mimeType"), "");
		int bv = cm_jat(j, cm_jget(j, l->root, "bufferViews"), cm_jget_int(j, im, "bufferView", -1));
		if (bv >= 0) {
			int buffer = cm_jget_int(j, bv, "buffer", 0);
			int offset = cm_jget_int(j, bv, "byteOffset", 0);
			int length = cm_jget_int(j, bv, "byteLength", 0);
			if (buffer < 0 || buffer >= asize(l->buffers) || !l->buffers[buffer]) continue;
			if (offset < 0 || length < 0 || (int64_t)offset + length > (int64_t)l->buffer_sizes[buffer]) continue;
			image->data = (uint8_t*)CM_ALLOC((size_t)(length ? length : 1));
			memcpy(image->data, l->buffers[buffer] + offset, (size_t)length);
			image->size = length;
		} else {
			// External or data-URI image file.
			const char* uri = cm_jstr(j, cm_jget(j, im, "uri"), NULL);
			if (!uri) continue;
			if (!strncmp(uri, "data:", 5)) {
				const char* comma = strchr(uri, ',');
				if (comma) image->data = cm_base64_decode(comma + 1, &image->size);
			} else {
				image->data = cm_read_external(l, uri, &image->size);
			}
		}
	}

	// Materials: the full pbrMetallicRoughness set plus KHR_texture_transform.
	int jmaterials = cm_jget(j, l->root, "materials");
	model->material_count = cm_jlen(j, jmaterials);
	if (model->material_count) {
		model->materials = (CM_Material*)CM_ALLOC(sizeof(CM_Material) * (size_t)model->material_count);
		memset(model->materials, 0, sizeof(CM_Material) * (size_t)model->material_count);
	}
	for (int i = 0; i < model->material_count; ++i) {
		int m = cm_jat(j, jmaterials, i);
		CM_Material* material = model->materials + i;
		material->name = cm_jstr(j, cm_jget(j, m, "name"), "");
		material->base_color[0] = material->base_color[1] = material->base_color[2] = material->base_color[3] = 1;
		material->metallic = 1;
		material->roughness = 1;
		material->normal_scale = 1;
		material->occlusion_strength = 1;
		material->alpha_cutoff = 0.5f;
		int pbr = cm_jget(j, m, "pbrMetallicRoughness");
		int factor = cm_jget(j, pbr, "baseColorFactor");
		for (int k = 0; k < 4 && k < cm_jlen(j, factor); ++k) material->base_color[k] = (float)cm_jnum(j, cm_jat(j, factor, k), 1);
		material->metallic = (float)cm_jnum(j, cm_jget(j, pbr, "metallicFactor"), 1);
		material->roughness = (float)cm_jnum(j, cm_jget(j, pbr, "roughnessFactor"), 1);
		material->base_color_texture = cm_load_texture_ref(l, cm_jget(j, pbr, "baseColorTexture"));
		material->metallic_roughness_texture = cm_load_texture_ref(l, cm_jget(j, pbr, "metallicRoughnessTexture"));
		int normal = cm_jget(j, m, "normalTexture");
		material->normal_texture = cm_load_texture_ref(l, normal);
		material->normal_scale = (float)cm_jnum(j, cm_jget(j, normal, "scale"), 1);
		int occlusion = cm_jget(j, m, "occlusionTexture");
		material->occlusion_texture = cm_load_texture_ref(l, occlusion);
		material->occlusion_strength = (float)cm_jnum(j, cm_jget(j, occlusion, "strength"), 1);
		int emissive = cm_jget(j, m, "emissiveFactor");
		for (int k = 0; k < 3 && k < cm_jlen(j, emissive); ++k) material->emissive[k] = (float)cm_jnum(j, cm_jat(j, emissive, k), 0);
		material->emissive_texture = cm_load_texture_ref(l, cm_jget(j, m, "emissiveTexture"));
		const char* alpha_mode = cm_jstr(j, cm_jget(j, m, "alphaMode"), "OPAQUE");
		material->alpha_mode = !strcmp(alpha_mode, "MASK") ? CM_ALPHA_MASK : !strcmp(alpha_mode, "BLEND") ? CM_ALPHA_BLEND : CM_ALPHA_OPAQUE;
		material->alpha_cutoff = (float)cm_jnum(j, cm_jget(j, m, "alphaCutoff"), 0.5);
		material->double_sided = cm_jint(j, cm_jget(j, m, "doubleSided"), 0);
		material->extras = cm_extras(l, m);
	}

	// Designer-placed lights (KHR_lights_punctual); nodes reference them by index.
	int jlights = cm_jget(j, cm_jget(j, cm_jget(j, l->root, "extensions"), "KHR_lights_punctual"), "lights");
	model->light_count = cm_jlen(j, jlights);
	if (model->light_count) {
		model->lights = (CM_Light*)CM_ALLOC(sizeof(CM_Light) * (size_t)model->light_count);
		memset(model->lights, 0, sizeof(CM_Light) * (size_t)model->light_count);
	}
	for (int i = 0; i < model->light_count; ++i) {
		int lt = cm_jat(j, jlights, i);
		CM_Light* light = model->lights + i;
		light->name = cm_jstr(j, cm_jget(j, lt, "name"), "");
		const char* type = cm_jstr(j, cm_jget(j, lt, "type"), "directional");
		light->type = !strcmp(type, "point") ? CM_LIGHT_POINT : !strcmp(type, "spot") ? CM_LIGHT_SPOT : CM_LIGHT_DIRECTIONAL;
		light->color[0] = light->color[1] = light->color[2] = 1;
		int color = cm_jget(j, lt, "color");
		for (int k = 0; k < 3 && k < cm_jlen(j, color); ++k) light->color[k] = (float)cm_jnum(j, cm_jat(j, color, k), 1);
		light->intensity = (float)cm_jnum(j, cm_jget(j, lt, "intensity"), 1);
		light->range = (float)cm_jnum(j, cm_jget(j, lt, "range"), 0);
		int spot = cm_jget(j, lt, "spot");
		light->inner_cone = (float)cm_jnum(j, cm_jget(j, spot, "innerConeAngle"), 0);
		light->outer_cone = (float)cm_jnum(j, cm_jget(j, spot, "outerConeAngle"), 0.7853981634); // pi/4 per spec.
	}

	// Cross-reference validation. Every index above came from the file, and callers index
	// straight into these arrays -- an out-of-range reference must never reach them.
	// Danglers that have a safe "absent" value become -1; a bad joint index does not (the
	// palette is positional, and silently posing the wrong bone is worse than refusing),
	// so that one fails the load.
	for (int i = 0; i < model->node_count; ++i) {
		CM_Node* node = model->nodes + i;
		if (node->mesh >= model->mesh_count) node->mesh = -1;
		if (node->skin >= model->skin_count) node->skin = -1;
		if (node->light >= model->light_count) node->light = -1;
	}
	for (int i = 0; i < model->mesh_count; ++i) {
		for (int p = 0; p < model->meshes[i].primitive_count; ++p) {
			CM_Primitive* prim = model->meshes[i].primitives + p;
			if (prim->material >= model->material_count) prim->material = -1;
		}
	}
	for (int i = 0; i < model->material_count; ++i) {
		CM_TextureRef* slots[5] = {
			&model->materials[i].base_color_texture, &model->materials[i].metallic_roughness_texture,
			&model->materials[i].normal_texture, &model->materials[i].occlusion_texture,
			&model->materials[i].emissive_texture,
		};
		for (int s = 0; s < 5; ++s) {
			if (slots[s]->image >= model->image_count) slots[s]->image = -1;
		}
	}
	for (int i = 0; i < model->skin_count; ++i) {
		for (int k = 0; k < model->skins[i].joint_count; ++k) {
			int joint = model->skins[i].joints[k];
			if (joint < 0 || joint >= model->node_count) { cm_free(model); CM_FAIL("skin %d joint %d is out of range", i, k); }
		}
	}

	return model;
}

CM_Model* cm_load(const void* data, int size)
{
	CM_LoadParams params;
	memset(&params, 0, sizeof(params));
	return cm_load_ex(data, size, params);
}

CM_Model* cm_load_ex(const void* data, int size, CM_LoadParams params)
{
	cm_g_error[0] = 0;
	const uint8_t* bytes = (const uint8_t*)data;
	CM_Loader l;
	memset(&l, 0, sizeof(l));
	l.params = params;
	const char* json_text = NULL;
	int json_size = 0;
	const uint8_t* bin = NULL;
	int bin_size = 0;

	if (size < 0) CM_FAIL("negative size");
	if (size >= 12 && !memcmp(bytes, "glTF", 4)) {
		// GLB container: 12-byte header then 8-byte-headed chunks, 4-byte aligned. Every
		// length here is attacker-controlled, so the whole walk runs in uint64 -- 32-bit
		// arithmetic on a hostile chunk_size wraps, and a wrapped offset reads wherever it
		// likes.
		uint32_t version, total;
		memcpy(&version, bytes + 4, 4);
		memcpy(&total, bytes + 8, 4);
		if (version != 2) CM_FAIL("GLB version %u (only 2 is supported)", version);
		if ((uint64_t)total > (uint64_t)size) CM_FAIL("GLB length field exceeds the provided data");
		uint64_t at = 12;
		uint64_t end = total;
		while (at + 8 <= end) {
			uint32_t chunk_size, chunk_type;
			memcpy(&chunk_size, bytes + at, 4);
			memcpy(&chunk_type, bytes + at + 4, 4);
			at += 8;
			if ((uint64_t)chunk_size > end - at) CM_FAIL("GLB chunk overruns the file");
			if (chunk_type == 0x4E4F534A) { json_text = (const char*)(bytes + at); json_size = (int)chunk_size; } // "JSON"
			else if (chunk_type == 0x004E4942) { bin = bytes + at; bin_size = (int)chunk_size; }                  // "BIN"
			uint64_t advance = ((uint64_t)chunk_size + 3u) & ~(uint64_t)3u;
			if (advance == 0) break; // Zero-length chunk: nothing more to walk.
			at += advance;
		}
		if (!json_text) CM_FAIL("GLB has no JSON chunk");
	} else {
		json_text = (const char*)bytes;
		json_size = size;
	}

	if (!cm_json_parse(&l.json, json_text, json_size)) {
		cm_json_free(&l.json);
		CM_FAIL("failed to parse glTF JSON");
	}
	l.root = 0;

	// Resolve buffers: buffer 0 may be the GLB binary chunk; data URIs decode; external
	// file URIs go through the read callback (this header does no IO of its own).
	int jbuffers = cm_jget(&l.json, l.root, "buffers");
	for (int i = 0; i < cm_jlen(&l.json, jbuffers); ++i) {
		int b = cm_jat(&l.json, jbuffers, i);
		const char* uri = cm_jstr(&l.json, cm_jget(&l.json, b, "uri"), NULL);
		uint8_t* ptr = NULL;
		int len = 0;
		int owned = 0;
		if (!uri) {
			ptr = (uint8_t*)bin;
			len = bin_size;
		} else if (!strncmp(uri, "data:", 5)) {
			const char* comma = strchr(uri, ',');
			if (comma) {
				ptr = cm_base64_decode(comma + 1, &len);
				owned = 1;
			}
		} else {
			ptr = cm_read_external(&l, uri, &len);
			owned = ptr != NULL;
		}
		apush(l.buffers, ptr);
		apush(l.buffer_sizes, len);
		apush(l.buffer_owned, owned);
	}

	CM_Model* model = cm_load_from_json(&l);

	for (int i = 0; i < asize(l.buffers); ++i) {
		if (l.buffer_owned[i] && l.buffers[i]) CM_FREE(l.buffers[i]);
	}
	afree(l.buffers);
	afree(l.buffer_sizes);
	afree(l.buffer_owned);
	cm_json_free(&l.json);
	return model;
}

CM_Animation* cm_find_animation(const CM_Model* model, const char* name)
{
	for (int i = 0; i < model->animation_count; ++i) {
		if (!strcmp(model->animations[i].name, name)) return model->animations + i;
	}
	return NULL;
}

//--------------------------------------------------------------------------------------------------
// Runtime.

void cm_rest_pose(const CM_Model* model, CM_Transform* locals)
{
	for (int i = 0; i < model->node_count; ++i) locals[i] = model->nodes[i].rest;
}

// Samples one channel at `time` into out[lanes], where lanes never exceeds
// ch->component_count. Handles the STEP/LINEAR/CUBICSPLINE layouts; rotations get
// shortest-path slerp (linear) or post-normalization (cubic), per spec.
static void cm_sample_channel(const CM_Channel* ch, float time, int lanes, float* out)
{
	int is_quat = ch->path == CM_PATH_ROTATION;
	// CUBICSPLINE keys are (in-tangent, value, out-tangent) triples; the value of key k
	// sits at element k*3 + 1.
	int cubic = ch->interpolation == CM_INTERP_CUBIC;
	int stride = cubic ? ch->component_count * 3 : ch->component_count;
	int value_offset = cubic ? ch->component_count : 0;
	if (ch->key_count == 1 || time <= ch->times[0]) {
		memcpy(out, ch->values + value_offset, sizeof(float) * (size_t)lanes);
		return;
	}
	if (time >= ch->times[ch->key_count - 1]) {
		memcpy(out, ch->values + (size_t)(ch->key_count - 1) * stride + value_offset, sizeof(float) * (size_t)lanes);
		return;
	}
	// Binary search for the bracketing pair.
	int lo = 0, hi = ch->key_count - 1;
	while (hi - lo > 1) {
		int mid = (lo + hi) >> 1;
		if (ch->times[mid] <= time) lo = mid;
		else hi = mid;
	}
	float span = ch->times[hi] - ch->times[lo];
	float t = span > 0 ? (time - ch->times[lo]) / span : 0;
	const float* a = ch->values + (size_t)lo * stride + value_offset;
	const float* b = ch->values + (size_t)hi * stride + value_offset;
	if (ch->interpolation == CM_INTERP_STEP) {
		memcpy(out, a, sizeof(float) * (size_t)lanes);
	} else if (cubic) {
		// glTF's Hermite basis: out-tangent of the left key, in-tangent of the right,
		// both scaled by the key span.
		const float* out_tan = a + ch->component_count; // Key lo's out-tangent trails its value.
		const float* in_tan = b - ch->component_count;  // Key hi's in-tangent precedes its value.
		float t2 = t * t, t3 = t2 * t;
		float h00 = 2 * t3 - 3 * t2 + 1;
		float h10 = t3 - 2 * t2 + t;
		float h01 = -2 * t3 + 3 * t2;
		float h11 = t3 - t2;
		for (int k = 0; k < lanes; ++k) {
			out[k] = h00 * a[k] + h10 * span * out_tan[k] + h01 * b[k] + h11 * span * in_tan[k];
		}
		if (is_quat) {
			float len = sqrtf(out[0] * out[0] + out[1] * out[1] + out[2] * out[2] + out[3] * out[3]);
			if (len > 0) { out[0] /= len; out[1] /= len; out[2] /= len; out[3] /= len; }
			else out[3] = 1;
		}
	} else if (is_quat) {
		cm_quat_slerp(a, b, t, out);
	} else {
		for (int k = 0; k < lanes; ++k) out[k] = a[k] + (b[k] - a[k]) * t;
	}
}

void cm_animate(const CM_Model* model, const CM_Animation* animation, float time, CM_Transform* locals)
{
	(void)model;
	for (int c = 0; c < animation->channel_count; ++c) {
		const CM_Channel* ch = animation->channels + c;
		if (ch->path == CM_PATH_WEIGHTS) continue; // See cm_animate_weights.
		float* target = ch->path == CM_PATH_TRANSLATION ? locals[ch->node].translation
		              : ch->path == CM_PATH_ROTATION ? locals[ch->node].rotation
		              : locals[ch->node].scale;
		cm_sample_channel(ch, time, ch->component_count, target);
	}
}

void cm_animate_weights(const CM_Model* model, const CM_Animation* animation, float time, int node, float* weights, int weight_count)
{
	(void)model;
	for (int c = 0; c < animation->channel_count; ++c) {
		const CM_Channel* ch = animation->channels + c;
		if (ch->path != CM_PATH_WEIGHTS || ch->node != node) continue;
		int lanes = weight_count < ch->component_count ? weight_count : ch->component_count;
		cm_sample_channel(ch, time, lanes, weights);
	}
}

int cm_find_node(const CM_Model* model, const char* name)
{
	for (int i = 0; i < model->node_count; ++i) {
		if (!strcmp(model->nodes[i].name, name)) return i;
	}
	return -1;
}

void cm_blend(const CM_Model* model, const CM_Transform* a, const CM_Transform* b, float t, CM_Transform* out)
{
	for (int i = 0; i < model->node_count; ++i) {
		for (int k = 0; k < 3; ++k) {
			out[i].translation[k] = a[i].translation[k] + (b[i].translation[k] - a[i].translation[k]) * t;
			out[i].scale[k] = a[i].scale[k] + (b[i].scale[k] - a[i].scale[k]) * t;
		}
		cm_quat_slerp(a[i].rotation, b[i].rotation, t, out[i].rotation);
	}
}

void cm_world_transforms(const CM_Model* model, const CM_Transform* locals, float* world)
{
	for (int i = 0; i < model->node_count; ++i) {
		int n = model->order[i];
		float local[16];
		cm_trs_to_mat4(locals + n, local);
		int parent = model->nodes[n].parent;
		if (parent < 0) memcpy(world + (size_t)n * 16, local, sizeof(local));
		else cm_mat4_mul(world + (size_t)parent * 16, local, world + (size_t)n * 16);
	}
}

void cm_skin_palette(const CM_Model* model, int skin_index, const float* world, float* palette)
{
	const CM_Skin* skin = model->skins + skin_index;
	for (int i = 0; i < skin->joint_count; ++i) {
		cm_mat4_mul(world + (size_t)skin->joints[i] * 16, skin->inverse_binds + (size_t)i * 16, palette + (size_t)i * 16);
	}
}

#endif // CUTE_MODEL_IMPLEMENTATION_ONCE
#endif // CUTE_MODEL_IMPLEMENTATION

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
	the public domain. We make this dedication for the benefit of the public at
	large and to the detriment of our heirs and successors. We intend this
	dedication to be an overt act of relinquishment in perpetuity of all
	present and future rights to this software under copyright law.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	------------------------------------------------------------------------------
*/
