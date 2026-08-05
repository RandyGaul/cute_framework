/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_DRAW3D_H
#define CF_DRAW3D_H

#include "cute_defines.h"
#include "cute_math3d.h"
#include "cute_graphics.h"
#include "cute_draw.h"
#include "cute_sprite.h"

//--------------------------------------------------------------------------------------------------
// C API
//
// Immediate-mode 3d mesh submission, sharing the 2d draw API's command stream.
//
// This layer owns transforms and submission -- you own every fragment. There is no built-in
// lighting, no material system, and no assumptions about how you shade: you write a shader with
// `cf_make_shader` (the two-file path, which is the one that accepts a custom vertex stage), push
// it, and submit meshes. Shadow maps, g-buffers, toon pipelines, or any pass architecture you like
// are all expressed the same way: push a shader and render state, submit meshes, `cf_render_to` a
// canvas, repeat.
//
// State is stack-based, exactly like cute_draw.h: cameras, transforms, shaders, render states and
// mesh attributes all push/pop/peek, so nesting is safe by construction. Meshes flow through the
// same command stream as sprites, shapes and text -- `cf_draw_push_layer` orders them against 2d
// drawing, `cf_render_to` flushes everything together, and `CF_DrawList` records 3d submissions
// just like 2d ones (see the DRAW LISTS section below).
//
// WHAT CARRIES OVER FROM cute_draw.h
//
// One rule decides it: state describing WHERE and WHEN a draw lands (stream structure) is shared;
// state describing HOW pixels are produced (shading) belongs to one domain or the other.
//
// Shared -- these cute_draw.h calls apply to mesh submissions too:
//
//     cf_draw_push_layer / pop / peek       Orders meshes against 2d drawing and each other.
//     cf_draw_push_scissor / pop / peek     Captured per submission, like everything else.
//     cf_draw_push_viewport / pop / peek
//     cf_make_draw_list, cf_draw_list_begin / end, cf_draw_list      Records + bakes meshes.
//     cf_render_to, cf_render_layers_to, cf_app_draw_onto_screen     The flush.
//
// Everything else in cute_draw.h is 2d-geometry-only: colors, blend modes, filter, alpha discard,
// shape antialiasing, tri/vertex attributes, text state, and the 2d camera never touch meshes.
// Four concepts exist on BOTH sides as parallel stacks with zero crossover -- the 2d and 3d
// versions are independent by design, and pushing one never affects the other:
//
//     cf_draw_push_shader        vs  cf_draw3d_push_shader
//     cf_draw_push_render_state  vs  cf_draw3d_push_render_state
//     cf_draw_set_uniform/_texture  vs  cf_draw3d_set_uniform/_texture
//     cf_draw_push (2d camera)   vs  cf_draw3d_push (3d transform)
//
// Instancing is automatic. Consecutive submissions of the same mesh under the same shader, render
// state, textures and uniforms coalesce into a single instanced draw at flush -- drawing one rock
// 500 times in a loop is one draw call, the same way 500 sprites are. You never ask for
// instancing; you just draw. Group same-mesh submissions together for the best coalescing, or
// record them into a `CF_DrawList`, whose bake groups them regardless of submission order.
//
// THE SHADER CONTRACT
//
// Because instancing is automatic, per-mesh data rides in instance-rate vertex attributes rather
// than uniforms. Your vertex shader declares the reserved inputs below (only the ones it uses)
// alongside your own mesh attributes, plus one required uniform member:
//
//     // Your mesh's own attributes, bound by name as usual (any layout you like):
//     layout (location = 0) in vec3 in_pos;
//     layout (location = 1) in vec3 in_normal;
//     layout (location = 2) in vec2 in_uv;
//
//     // Reserved, fed by this layer per mesh submission:
//     layout (location = 8)  in vec4 in_model0;          // model transform, affine row 0
//     layout (location = 9)  in vec4 in_model1;          // row 1
//     layout (location = 10) in vec4 in_model2;          // row 2
//     layout (location = 11) in vec4 in_uv_rect;         // cf_draw3d_push_texture's atlas sub-rect
//     layout (location = 12) in vec4 in_nmat0;           // normal matrix row 0 (optional)
//     layout (location = 13) in vec4 in_nmat1;           // row 1
//     layout (location = 14) in vec4 in_nmat2;           // row 2
//     layout (location = 15) in vec4 in_mesh_attributes; // cf_draw3d_push_mesh_attributes
//
//     layout (set = 1, binding = 0) uniform uniform_block {
//         mat4 u_view_projection;                        // set by this layer from the camera stacks
//         // ... your own vertex uniforms ...
//     };
//
//     void main()
//     {
//         vec4 p = vec4(in_pos, 1.0);
//         vec3 world_pos = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));
//         vec3 world_normal = normalize(vec3(dot(in_nmat0.xyz, in_normal),
//                                            dot(in_nmat1.xyz, in_normal),
//                                            dot(in_nmat2.xyz, in_normal)));
//         gl_Position = u_view_projection * vec4(world_pos, 1.0);
//     }
//
// The model rows are the composed transform stack at submission time, stored as three rows of the
// affine 4x3 (the fourth row is always 0,0,0,1). Everything after `gl_Position` -- lighting,
// shadowing, whatever -- is yours.
//
// DRAW LISTS
//
// `cf_make_draw_list` / `cf_draw_list_begin` / `cf_draw_list_end` / `cf_draw_list` record and
// replay 3d submissions just like 2d drawing. For mesh commands, `cf_draw_list_end` additionally
// bakes: submissions are grouped by (mesh, shader, render state, textures, uniforms), each group's
// transforms and mesh attributes are written into an instance buffer once, and replay issues one
// instanced draw per group -- record a 2,000-object level of 6 unique meshes at load time and it
// replays as 6 draws with zero per-object CPU cost. Replay composes the then-current camera and
// transform stacks, so a recorded level renders under a live camera and can be moved as a whole
// for free. Baked lists also get exact per-instance normal matrices (computed once at bake); the
// immediate path derives normals from the model rows, which is exact for rigid transforms and
// uniform scale.
//
// ESCAPE HATCH
//
// A submitted mesh that has its own instance buffer (`cf_mesh_set_instance_buffer`) is drawn
// as-is: no reserved attributes are bound, no coalescing happens, and instancing is entirely
// yours. The low-level path in cute_graphics.h remains fully available for cases this layer does
// not fit.

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// DIAGNOSTICS
//
// Instancing here is automatic, which means it is also invisible: submissions coalesce silently,
// and so do the state changes that stop them from coalescing. `cf_draw3d_stats` makes that
// legible -- how many draw calls your submissions actually became, and, for each one that failed
// to join the batch before it, which piece of state broke it.
//
//     CF_DrawStats3d stats = cf_draw3d_stats();
//     printf("%d instances -> %d draws (%s)\n", stats.instances, stats.commands,
//         cf_draw_split_3d_to_string(cf_draw3d_worst_split(&stats)));

/**
 * @enum     CF_DrawSplit3d
 * @category draw3d
 * @brief    Why one mesh submission could not join the previous one's instanced draw.
 * @related  CF_DrawStats3d cf_draw3d_stats cf_draw_split_3d_to_string
 */
#define CF_DRAW_SPLIT_3D_DEFS \
	/* @entry Nothing broke -- the submission coalesced. Never counted. */ \
	CF_ENUM(DRAW_SPLIT_3D_NONE,         0) \
	/* @entry Nothing to join: the first submission, or 2d drawing came between. */ \
	CF_ENUM(DRAW_SPLIT_3D_FIRST,        1) \
	/* @entry A different mesh. Group submissions by mesh, or bake a draw list. */ \
	CF_ENUM(DRAW_SPLIT_3D_MESH,         2) \
	/* @entry A different shader was pushed. */ \
	CF_ENUM(DRAW_SPLIT_3D_SHADER,       3) \
	/* @entry A different render state was pushed. */ \
	CF_ENUM(DRAW_SPLIT_3D_RENDER_STATE, 4) \
	/* @entry Uniforms or textures changed. Prefer mesh attributes for per-object data. */ \
	CF_ENUM(DRAW_SPLIT_3D_UNIFORMS,     5) \
	/* @entry The camera changed (projection or view). */ \
	CF_ENUM(DRAW_SPLIT_3D_CAMERA,       6) \
	/* @entry A different layer. */ \
	CF_ENUM(DRAW_SPLIT_3D_LAYER,        7) \
	/* @entry A different scissor rect. */ \
	CF_ENUM(DRAW_SPLIT_3D_SCISSOR,      8) \
	/* @entry A different viewport. */ \
	CF_ENUM(DRAW_SPLIT_3D_VIEWPORT,     9) \
	/* @entry Sprite texturing was toggled on or off. */ \
	CF_ENUM(DRAW_SPLIT_3D_TEXTURE,      10) \
	/* @entry The escape hatch: a mesh with its own instance buffer never coalesces. */ \
	CF_ENUM(DRAW_SPLIT_3D_ESCAPE,       11) \
	/* @end */

typedef enum CF_DrawSplit3d
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_DRAW_SPLIT_3D_DEFS
	#undef CF_ENUM
} CF_DrawSplit3d;

#define CF_DRAW_SPLIT_3D_COUNT 12

/**
 * @function cf_draw_split_3d_to_string
 * @category draw3d
 * @brief    Returns a `CF_DrawSplit3d` as a human-readable string.
 * @related  CF_DrawSplit3d cf_draw3d_stats
 */
CF_INLINE const char* cf_draw_split_3d_to_string(CF_DrawSplit3d split)
{
	switch (split) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_DRAW_SPLIT_3D_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @struct   CF_DrawStats3d
 * @category draw3d
 * @brief    What your 3d submissions became, and why.
 * @related  cf_draw3d_stats cf_draw3d_worst_split CF_DrawSplit3d
 */
typedef struct CF_DrawStats3d
{
	/* @member Mesh instances submitted. */
	int instances;

	/* @member Draw calls those instances became. Equal to `instances` means nothing batched. */
	int commands;

	/* @member Per-reason tally of batch breaks, indexed by `CF_DrawSplit3d`. Sums to `commands`. */
	int splits[CF_DRAW_SPLIT_3D_COUNT];
} CF_DrawStats3d;
// @end

/**
 * @function cf_draw3d_stats
 * @category draw3d
 * @brief    Returns and resets the 3d submission counters.
 * @remarks  Call once per frame, after `cf_app_draw_onto_screen`. Counters accumulate over
 *           whatever interval you leave between calls.
 * @related  CF_DrawStats3d cf_draw3d_worst_split cf_draw_split_3d_to_string
 */
CF_API CF_DrawStats3d CF_CALL cf_draw3d_stats(void);

/**
 * @function cf_draw3d_worst_split
 * @category draw3d
 * @brief    Returns the state change that broke the most batches.
 * @param    stats  Stats from `cf_draw3d_stats`.
 * @remarks  The one thing to fix first. `CF_DRAW_SPLIT_3D_FIRST` is not a problem -- it just
 *           counts batches that had nothing to join.
 * @related  CF_DrawStats3d cf_draw3d_stats cf_draw_split_3d_to_string
 */
CF_INLINE CF_DrawSplit3d cf_draw3d_worst_split(const CF_DrawStats3d* stats)
{
	int best = 0, best_count = -1;
	for (int i = 0; i < CF_DRAW_SPLIT_3D_COUNT; ++i) {
		if (i == CF_DRAW_SPLIT_3D_NONE || i == CF_DRAW_SPLIT_3D_FIRST) continue;
		if (stats->splits[i] > best_count) { best_count = stats->splits[i]; best = i; }
	}
	return (CF_DrawSplit3d)best;
}

//--------------------------------------------------------------------------------------------------
// Camera.

/**
 * @function cf_draw3d_push_projection
 * @category draw3d
 * @brief    Pushes the projection matrix used for 3d mesh submissions.
 * @param    projection  The projection matrix, e.g. from `cf_perspective` or `cf_ortho`.
 * @remarks  Defaults to the identity matrix, so nothing sensible renders until you push a real
 *           projection. `cf_perspective` and `cf_ortho` produce matrices with the clip conventions
 *           this layer (and SDL_GPU) expects -- clip-space z in [0, 1]. The 2d projection set by
 *           `cf_draw_projection` is unrelated and does not affect meshes.
 * @related  cf_draw3d_push_projection cf_draw3d_pop_projection cf_draw3d_peek_projection cf_draw3d_push_view cf_perspective cf_ortho
 */
CF_API void CF_CALL cf_draw3d_push_projection(CF_M4x4 projection);

/**
 * @function cf_draw3d_pop_projection
 * @category draw3d
 * @brief    Pops and returns the last projection matrix.
 * @related  cf_draw3d_push_projection cf_draw3d_pop_projection cf_draw3d_peek_projection
 */
CF_API CF_M4x4 CF_CALL cf_draw3d_pop_projection(void);

/**
 * @function cf_draw3d_peek_projection
 * @category draw3d
 * @brief    Returns the current projection matrix.
 * @related  cf_draw3d_push_projection cf_draw3d_pop_projection cf_draw3d_peek_projection
 */
CF_API CF_M4x4 CF_CALL cf_draw3d_peek_projection(void);

/**
 * @function cf_draw3d_push_view
 * @category draw3d
 * @brief    Pushes the view matrix used for 3d mesh submissions.
 * @param    view  The view matrix, e.g. from `cf_look_at`.
 * @remarks  Defaults to the identity matrix. Together with the projection this forms
 *           `u_view_projection`, the one uniform member the shader contract requires. Cameras are
 *           live for draw-list replay: a recorded level renders under whatever view is current at
 *           `cf_draw_list` time.
 * @related  cf_draw3d_push_view cf_draw3d_pop_view cf_draw3d_peek_view cf_draw3d_push_projection cf_look_at
 */
CF_API void CF_CALL cf_draw3d_push_view(CF_M4x4 view);

/**
 * @function cf_draw3d_pop_view
 * @category draw3d
 * @brief    Pops and returns the last view matrix.
 * @related  cf_draw3d_push_view cf_draw3d_pop_view cf_draw3d_peek_view
 */
CF_API CF_M4x4 CF_CALL cf_draw3d_pop_view(void);

/**
 * @function cf_draw3d_peek_view
 * @category draw3d
 * @brief    Returns the current view matrix.
 * @related  cf_draw3d_push_view cf_draw3d_pop_view cf_draw3d_peek_view
 */
CF_API CF_M4x4 CF_CALL cf_draw3d_peek_view(void);

//--------------------------------------------------------------------------------------------------
// Model transform stack. The current coordinate system for mesh submissions, exactly like the 2d
// draw API's transform, composed incrementally and captured per submission.

/**
 * @function cf_draw3d_push
 * @category draw3d
 * @brief    Saves a copy of the current 3d transform onto the stack.
 * @remarks  Use `cf_draw3d_pop` to restore it. This is the 3d analog of `cf_draw_push`; the two
 *           stacks are independent, and the 2d transform never affects meshes.
 * @related  cf_draw3d_push cf_draw3d_pop cf_draw3d_translate cf_draw3d_rotate cf_draw3d_scale cf_draw3d_transform
 */
CF_API void CF_CALL cf_draw3d_push(void);

/**
 * @function cf_draw3d_pop
 * @category draw3d
 * @brief    Restores the last saved 3d transform.
 * @related  cf_draw3d_push cf_draw3d_pop cf_draw3d_translate cf_draw3d_rotate cf_draw3d_scale cf_draw3d_transform
 */
CF_API void CF_CALL cf_draw3d_pop(void);

/**
 * @function cf_draw3d_translate
 * @category draw3d
 * @brief    Translates the current 3d coordinate system.
 * @param    t  The translation.
 * @related  cf_draw3d_push cf_draw3d_pop cf_draw3d_translate cf_draw3d_rotate cf_draw3d_scale cf_draw3d_transform
 */
CF_API void CF_CALL cf_draw3d_translate(CF_V3 t);

/**
 * @function cf_draw3d_rotate
 * @category draw3d
 * @brief    Rotates the current 3d coordinate system.
 * @param    q  The rotation. Should be unit length -- see `cf_quat_norm`.
 * @related  cf_draw3d_push cf_draw3d_pop cf_draw3d_translate cf_draw3d_rotate cf_draw3d_scale cf_draw3d_transform cf_quat_from_axis_angle
 */
CF_API void CF_CALL cf_draw3d_rotate(CF_Quat q);

/**
 * @function cf_draw3d_scale
 * @category draw3d
 * @brief    Scales the current 3d coordinate system.
 * @param    s  The per-axis scale. `cf_v3(2.0f)` splats a uniform scale.
 * @remarks  Non-uniform scale is handled correctly by baked draw lists (exact per-instance normal
 *           matrices); the immediate path derives normals from the model rows, which skews under
 *           non-uniform scale. See the shader contract at the top of this header.
 * @related  cf_draw3d_push cf_draw3d_pop cf_draw3d_translate cf_draw3d_rotate cf_draw3d_scale cf_draw3d_transform
 */
CF_API void CF_CALL cf_draw3d_scale(CF_V3 s);

/**
 * @function cf_draw3d_transform
 * @category draw3d
 * @brief    Composes a matrix onto the current 3d coordinate system.
 * @param    m  The transform to apply.
 * @related  cf_draw3d_push cf_draw3d_pop cf_draw3d_translate cf_draw3d_rotate cf_draw3d_scale cf_draw3d_transform
 */
CF_API void CF_CALL cf_draw3d_transform(CF_M4x4 m);

/**
 * @function cf_draw3d_peek_transform
 * @category draw3d
 * @brief    Returns the current composed 3d transform.
 * @related  cf_draw3d_push cf_draw3d_pop cf_draw3d_transform cf_draw3d_mul
 */
CF_API CF_M4x4 CF_CALL cf_draw3d_peek_transform(void);

/**
 * @function cf_draw3d_mul
 * @category draw3d
 * @brief    Applies the current 3d transform to a point.
 * @param    p  The point to transform.
 * @related  cf_draw3d_peek_transform cf_draw3d_transform cf_m4_transform_point
 */
CF_API CF_V3 CF_CALL cf_draw3d_mul(CF_V3 p);

//--------------------------------------------------------------------------------------------------
// Shader and pipeline state.

/**
 * @function cf_draw3d_push_shader
 * @category draw3d
 * @brief    Pushes the shader used for subsequent mesh submissions.
 * @param    shader  A shader from `cf_make_shader` (the two-file path), following the shader
 *                   contract at the top of this header.
 * @remarks  There is no default 3d shader -- submitting a mesh without a shader pushed is an
 *           error. This is deliberate: this layer makes no assumptions about how you shade.
 *           Shaders under `cf_shader_directory` hot-reload automatically while you edit.
 *
 *           Built-in shapes interpret the top of this stack by the shader's kind:
 *           a shape shader (`cf_make_draw3d_shape_shader`) post-processes the built-in
 *           stroke/solid result through its stub; a plain shader fully replaces the solid
 *           pipeline (the ordinary mesh contract) and is ignored by strokes, whose ribbon
 *           vertex stage and distance-field fragment are built in; with nothing pushed,
 *           shapes use their built-ins.
 * @related  CF_Shader cf_make_shader cf_make_draw3d_shape_shader cf_draw3d_push_shader cf_draw3d_pop_shader cf_draw3d_peek_shader cf_draw3d_mesh
 */
CF_API void CF_CALL cf_draw3d_push_shader(CF_Shader shader);

/**
 * @function cf_draw3d_pop_shader
 * @category draw3d
 * @brief    Pops and returns the last pushed shader.
 * @related  cf_draw3d_push_shader cf_draw3d_pop_shader cf_draw3d_peek_shader
 */
CF_API CF_Shader CF_CALL cf_draw3d_pop_shader(void);

/**
 * @function cf_draw3d_peek_shader
 * @category draw3d
 * @brief    Returns the current shader.
 * @related  cf_draw3d_push_shader cf_draw3d_pop_shader cf_draw3d_peek_shader
 */
CF_API CF_Shader CF_CALL cf_draw3d_peek_shader(void);

/**
 * @function cf_draw3d_push_render_state
 * @category draw3d
 * @brief    Pushes the render state used for subsequent mesh submissions.
 * @param    render_state  The render state. See `CF_RenderState`.
 * @remarks  Defaults to `cf_render_state_3d_defaults` -- depth writes on, `LESS_THAN` depth test,
 *           back-face culling of clockwise faces (submit meshes with counter-clockwise winding).
 *           Remember depth state only functions on a canvas created with `depth_stencil_enable`.
 * @related  CF_RenderState cf_render_state_3d_defaults cf_draw3d_push_render_state cf_draw3d_pop_render_state cf_draw3d_peek_render_state
 */
CF_API void CF_CALL cf_draw3d_push_render_state(CF_RenderState render_state);

/**
 * @function cf_draw3d_pop_render_state
 * @category draw3d
 * @brief    Pops and returns the last render state.
 * @related  cf_draw3d_push_render_state cf_draw3d_pop_render_state cf_draw3d_peek_render_state
 */
CF_API CF_RenderState CF_CALL cf_draw3d_pop_render_state(void);

/**
 * @function cf_draw3d_peek_render_state
 * @category draw3d
 * @brief    Returns the current render state.
 * @related  cf_draw3d_push_render_state cf_draw3d_pop_render_state cf_draw3d_peek_render_state
 */
CF_API CF_RenderState CF_CALL cf_draw3d_peek_render_state(void);

//--------------------------------------------------------------------------------------------------
// Shader plumbing, by name -- the same idiom as the 2d draw API. Values are captured per
// submission; changing a uniform between two submissions of the same mesh splits their coalescing
// group, which is the lever for material variety within one shader.

/**
 * @function cf_draw3d_set_uniform
 * @category draw3d
 * @brief    Sets a uniform for subsequent mesh submissions, by name.
 * @param    name          The uniform's name in the shader.
 * @param    data          A pointer to the data to send.
 * @param    type          The `CF_UniformType` of the data.
 * @param    array_length  The number of elements to send.
 * @remarks  `u_view_projection` is reserved -- it is set from the camera stacks and cannot be
 *           overridden here.
 * @related  cf_draw3d_set_uniform cf_draw3d_set_uniform_int cf_draw3d_set_uniform_float cf_draw3d_set_uniform_v2 cf_draw3d_set_uniform_v3 cf_draw3d_set_uniform_m4 cf_draw3d_set_uniform_color cf_draw3d_set_texture
 */
CF_API void CF_CALL cf_draw3d_set_uniform(const char* name, void* data, CF_UniformType type, int array_length);

/**
 * @function cf_draw3d_set_uniform_int
 * @category draw3d
 * @brief    Sets an integer uniform by name.
 * @related  cf_draw3d_set_uniform cf_draw3d_set_uniform_int cf_draw3d_set_uniform_float cf_draw3d_set_uniform_v2 cf_draw3d_set_uniform_v3 cf_draw3d_set_uniform_m4 cf_draw3d_set_uniform_color
 */
CF_API void CF_CALL cf_draw3d_set_uniform_int(const char* name, int val);

/**
 * @function cf_draw3d_set_uniform_float
 * @category draw3d
 * @brief    Sets a float uniform by name.
 * @related  cf_draw3d_set_uniform cf_draw3d_set_uniform_int cf_draw3d_set_uniform_float cf_draw3d_set_uniform_v2 cf_draw3d_set_uniform_v3 cf_draw3d_set_uniform_m4 cf_draw3d_set_uniform_color
 */
CF_API void CF_CALL cf_draw3d_set_uniform_float(const char* name, float val);

/**
 * @function cf_draw3d_set_uniform_v2
 * @category draw3d
 * @brief    Sets a `CF_V2` uniform by name.
 * @related  cf_draw3d_set_uniform cf_draw3d_set_uniform_int cf_draw3d_set_uniform_float cf_draw3d_set_uniform_v2 cf_draw3d_set_uniform_v3 cf_draw3d_set_uniform_m4 cf_draw3d_set_uniform_color
 */
CF_API void CF_CALL cf_draw3d_set_uniform_v2(const char* name, CF_V2 val);

/**
 * @function cf_draw3d_set_uniform_v3
 * @category draw3d
 * @brief    Sets a `CF_V3` uniform by name.
 * @related  cf_draw3d_set_uniform cf_draw3d_set_uniform_int cf_draw3d_set_uniform_float cf_draw3d_set_uniform_v2 cf_draw3d_set_uniform_v3 cf_draw3d_set_uniform_m4 cf_draw3d_set_uniform_color
 */
CF_API void CF_CALL cf_draw3d_set_uniform_v3(const char* name, CF_V3 val);

/**
 * @function cf_draw3d_set_uniform_m4
 * @category draw3d
 * @brief    Sets a `CF_M4x4` uniform by name.
 * @remarks  Useful for pass-specific matrices your shading needs -- a shadow-map matrix, a previous
 *           frame's view-projection for motion vectors, and so on.
 * @related  cf_draw3d_set_uniform cf_draw3d_set_uniform_int cf_draw3d_set_uniform_float cf_draw3d_set_uniform_v2 cf_draw3d_set_uniform_v3 cf_draw3d_set_uniform_m4 cf_draw3d_set_uniform_color
 */
CF_API void CF_CALL cf_draw3d_set_uniform_m4(const char* name, CF_M4x4 val);

/**
 * @function cf_draw3d_set_uniform_color
 * @category draw3d
 * @brief    Sets a color uniform by name.
 * @related  cf_draw3d_set_uniform cf_draw3d_set_uniform_int cf_draw3d_set_uniform_float cf_draw3d_set_uniform_v2 cf_draw3d_set_uniform_v3 cf_draw3d_set_uniform_m4 cf_draw3d_set_uniform_color
 */
CF_API void CF_CALL cf_draw3d_set_uniform_color(const char* name, CF_Color val);

/**
 * @function cf_draw3d_set_texture
 * @category draw3d
 * @brief    Binds a texture for subsequent mesh submissions, by name.
 * @param    name     The name of the sampler uniform in the shader.
 * @param    texture  The texture to bind.
 * @remarks  Bind a canvas's targets to sample a previous pass: `cf_canvas_get_target` for color,
 *           `cf_canvas_get_depth_stencil_target` for depth. When mapping clip-space positions to
 *           canvas uvs (shadow maps), v runs top-down: `uv = vec2(ndc.x, -ndc.y) * 0.5 + 0.5` on
 *           every backend. See the draw3d sample for a complete shadow-mapped scene.
 * @related  cf_draw3d_set_texture cf_draw3d_set_uniform CF_Texture cf_canvas_get_target cf_canvas_get_depth_stencil_target
 */
CF_API void CF_CALL cf_draw3d_set_texture(const char* name, CF_Texture texture);

/**
 * @function cf_draw3d_set_vs_storage_buffers
 * @category draw3d
 * @brief    Binds storage buffers to the vertex stage for subsequent mesh submissions.
 * @param    buffers  The buffers, in shader binding order (see `cf_apply_vs_storage_buffers`
 *                    for the declaration contract: `set = 0`, bindings after any vertex-stage
 *                    samplers). Up to 4.
 * @param    count    Number of buffers; 0 clears the set.
 * @remarks  The whole set replaces at once and is captured per submission like uniforms;
 *           identical sets keep coalescing alive. This is the skinning-at-scale tool: put
 *           every character's bone palette in ONE buffer, put each submission's palette
 *           base index in a spare `cf_draw3d_push_mesh_attributes` lane, and N animated
 *           characters coalesce into a single instanced draw:
 *
 *           ```glsl
 *           // Runtime array tails must be scalars or vectors, so a mat4 palette rides as
 *           // vec4 columns (each matrix is 4 consecutive vec4s, column-major):
 *           layout (std430, set = 0, binding = 0) readonly buffer bones_buffer { vec4 u_bones[]; };
 *           mat4 bone(uint j) { uint i = j * 4u; return mat4(u_bones[i], u_bones[i + 1u], u_bones[i + 2u], u_bones[i + 3u]); }
 *           // ...
 *           uint base = uint(in_mesh_attributes.x);
 *           mat4 skin = bone(base + in_joints.x) * in_weights.x + ...;
 *           ```
 *
 *           Note baked draw lists freeze their captures at record time -- a skinned mesh
 *           recorded into a `CF_DrawList` replays whatever palette buffer contents exist at
 *           draw time (the buffer binding is captured, its CONTENTS are not), so animated
 *           skinning composes with baked lists exactly when you keep updating the buffer.
 *           Works on every backend: GLES3/web emulates read-only storage buffers through
 *           texture fetches transparently.
 * @related  cf_apply_vs_storage_buffers CF_StorageBuffer cf_make_storage_buffer cf_draw3d_push_mesh_attributes
 */
CF_API void CF_CALL cf_draw3d_set_vs_storage_buffers(CF_StorageBuffer* buffers, int count);

//--------------------------------------------------------------------------------------------------
// Sprite-textured meshes. Meshes can be textured straight from CF's sprite/texture economy: the
// image lives wherever the atlas compiler decides, the sub-rect rides the `in_uv_rect` instance
// lane, and the shader samples `texture(u_image, mix(in_uv_rect.xy, in_uv_rect.zw, in_uv))`.
// There is no atlas API to hold correctly -- no pages, rects or pinning -- and drawing meshes
// together is itself the signal the atlas compiler uses to pack their images together, so draw
// calls converge downward as it learns the scene. Baked draw lists store image ids and refresh
// their uv_rect lanes only when the atlas reshuffles.
//
// Mesh UVs must lie in [0, 1]: hardware wrap cannot tile inside an atlas sub-rect. Meshes with
// tiling UVs bind a standalone `CF_Texture` via `cf_draw3d_set_texture` instead.

/**
 * @function cf_draw3d_push_texture
 * @category draw3d
 * @brief    Textures subsequent mesh submissions with a sprite's current frame.
 * @param    sprite  The sprite whose current frame supplies the image.
 * @remarks  The image participates in CF's texture atlas exactly like 2d sprite drawing, so many
 *           meshes with many different images still coalesce into few instanced draws -- and an
 *           animated sprite animates on the mesh. The shader receives the atlas page as `u_image`
 *           and the frame's sub-rect as `in_uv_rect`; mesh UVs must lie in [0, 1]. For tiling UVs
 *           or hand-managed textures use `cf_draw3d_set_texture` instead.
 * @related  cf_draw3d_push_texture cf_draw3d_pop_texture cf_draw3d_peek_texture cf_draw3d_set_texture cf_draw3d_mesh CF_Sprite
 */
CF_API void CF_CALL cf_draw3d_push_texture(const CF_Sprite* sprite);

/**
 * @function cf_draw3d_pop_texture
 * @category draw3d
 * @brief    Pops the last sprite texture; submissions revert to the previous one (or none).
 * @related  cf_draw3d_push_texture cf_draw3d_pop_texture cf_draw3d_peek_texture
 */
CF_API void CF_CALL cf_draw3d_pop_texture(void);

/**
 * @function cf_draw3d_peek_texture
 * @category draw3d
 * @brief    Returns the current sprite used for texturing, or `NULL` when none is pushed.
 * @related  cf_draw3d_push_texture cf_draw3d_pop_texture cf_draw3d_peek_texture
 */
CF_API const CF_Sprite* CF_CALL cf_draw3d_peek_texture(void);

//--------------------------------------------------------------------------------------------------
// Mesh attributes. One vec4 of per-submission data, delivered to the shader as
// `in_mesh_attributes` -- per-mesh tint, random seed, uv offset, or whatever your shading wants.
// The 3d member of the granularity family: `cf_draw_push_vertex_attributes` is per-vertex,
// `cf_draw_push_tri_attributes` per-triangle, this is per-mesh.

/**
 * @function cf_draw3d_push_mesh_attributes
 * @category draw3d
 * @brief    Pushes a vec4 of data attached to each subsequent mesh submission.
 * @param    attributes  The value delivered to the shader as `in_mesh_attributes`.
 * @remarks  Defaults to `(0, 0, 0, 0)`. Unlike a uniform, differing mesh attributes do NOT split
 *           coalescing -- 500 trees with 500 different tints are still one instanced draw. This is
 *           the intended way to vary appearance across many copies of one mesh.
 * @related  cf_draw3d_push_mesh_attributes cf_draw3d_pop_mesh_attributes cf_draw3d_peek_mesh_attributes cf_draw3d_mesh
 */
CF_API void CF_CALL cf_draw3d_push_mesh_attributes(CF_V4 attributes);

/**
 * @function cf_draw3d_pop_mesh_attributes
 * @category draw3d
 * @brief    Pops and returns the last mesh attributes.
 * @related  cf_draw3d_push_mesh_attributes cf_draw3d_pop_mesh_attributes cf_draw3d_peek_mesh_attributes
 */
CF_API CF_V4 CF_CALL cf_draw3d_pop_mesh_attributes(void);

/**
 * @function cf_draw3d_peek_mesh_attributes
 * @category draw3d
 * @brief    Returns the current mesh attributes.
 * @related  cf_draw3d_push_mesh_attributes cf_draw3d_pop_mesh_attributes cf_draw3d_peek_mesh_attributes
 */
CF_API CF_V4 CF_CALL cf_draw3d_peek_mesh_attributes(void);

/**
 * @function cf_draw3d_push_mesh_attributes2
 * @category draw3d
 * @brief    Pushes a second vec4 of per-submission data, delivered as `in_uv_rect`.
 * @param    attributes  The value delivered to the shader as `in_uv_rect`.
 * @remarks  The `in_uv_rect` lane carries the atlas sub-rect while a sprite texture is pushed
 *           (`cf_draw3d_push_texture`); with none pushed it is free, so it doubles as a second
 *           user attributes lane at zero extra bandwidth -- material indices, a second tint, ocean
 *           phase, whatever your shader wants. Defaults to `(0, 0, 1, 1)` (the full uv rect), so
 *           shaders that sample `u_image` against the white fallback keep working. Like
 *           `cf_draw3d_push_mesh_attributes`, differing values never split coalescing. Pushing a
 *           sprite texture overrides this lane for those submissions.
 * @related  cf_draw3d_push_mesh_attributes cf_draw3d_pop_mesh_attributes2 cf_draw3d_peek_mesh_attributes2 cf_draw3d_push_texture
 */
CF_API void CF_CALL cf_draw3d_push_mesh_attributes2(CF_V4 attributes);

/**
 * @function cf_draw3d_pop_mesh_attributes2
 * @category draw3d
 * @brief    Pops and returns the last second-lane mesh attributes.
 * @related  cf_draw3d_push_mesh_attributes2 cf_draw3d_peek_mesh_attributes2
 */
CF_API CF_V4 CF_CALL cf_draw3d_pop_mesh_attributes2(void);

/**
 * @function cf_draw3d_peek_mesh_attributes2
 * @category draw3d
 * @brief    Returns the current second-lane mesh attributes.
 * @related  cf_draw3d_push_mesh_attributes2 cf_draw3d_pop_mesh_attributes2
 */
CF_API CF_V4 CF_CALL cf_draw3d_peek_mesh_attributes2(void);

//--------------------------------------------------------------------------------------------------
// Submission.

/**
 * @function cf_draw3d_mesh
 * @category draw3d
 * @brief    Submits a mesh under the current 3d state.
 * @param    mesh  The mesh, from `cf_make_mesh`. Any vertex layout; attributes bind to your
 *                 shader's inputs by name.
 * @remarks  The current transform, mesh attributes, shader, render state, uniforms and textures
 *           are captured with the submission. Consecutive submissions differing only by transform
 *           and mesh attributes coalesce into one instanced draw automatically -- group same-mesh
 *           submissions together (or record them into a `CF_DrawList`, which groups at bake
 *           regardless of order). Meshes interleave with 2d drawing in the shared command stream;
 *           `cf_draw_push_layer` orders them.
 *
 *           Within a layer, ordering is automatic: depth-writing (opaque) submissions render
 *           first in submission order -- the depth test owns them -- and non-writing
 *           (translucent) submissions render after, sorted back-to-front by each submission's
 *           transform position, so overlapping glass, glows, and particles composite correctly
 *           no matter the order you submit them in. One caveat: translucent submissions of the
 *           SAME mesh under the same state coalesce into one draw and keep submission order
 *           within it -- when exact ordering between many copies of one translucent mesh
 *           matters, submit them roughly far-to-near or split them across layers.
 *
 *           Winding is counter-clockwise for front faces under the default render
 *           state. A mesh carrying its own instance buffer is drawn as-is with no reserved
 *           attributes bound -- see the escape hatch note at the top of this header.
 * @related  CF_Mesh cf_make_mesh cf_draw3d_push_shader cf_draw3d_transform cf_draw3d_push_mesh_attributes cf_render_to cf_make_draw_list
 */
CF_API void CF_CALL cf_draw3d_mesh(CF_Mesh mesh);

/**
 * @function cf_draw3d_mesh_range
 * @category draw3d
 * @brief    Submits a contiguous element range of a mesh under the current 3d state.
 * @param    mesh           The mesh, from `cf_make_mesh`.
 * @param    first_element  The first index (indexed meshes) or first vertex (non-indexed) to draw.
 * @param    element_count  How many indices or vertices to draw.
 * @remarks  The geometry-arena companion to `cf_draw3d_mesh`: pack many small meshes into one
 *           `CF_Mesh` and submit each as a range. Because every submission shares the same mesh,
 *           interleaving different ranges no longer splits the batch -- range submissions under
 *           the same state coalesce into ONE command that binds everything once and issues one
 *           tight range draw per contiguous record, where the same interleaving as separate
 *           meshes would cost a full state rebind per submission. Consecutive submissions of the
 *           SAME range still collapse into a single instanced draw, exactly like `cf_draw3d_mesh`.
 *
 *           An indexed arena writes its indices absolute into the shared vertex buffer -- there
 *           is deliberately no base-vertex offset (it would not port to GLES3). Ranges do not
 *           compose with `cf_draw3d_push_texture`'s sprite texturing. Ordering, translucency
 *           sorting, and the escape hatch behave exactly as documented on `cf_draw3d_mesh`.
 * @related  cf_draw3d_mesh CF_Mesh cf_make_mesh cf_draw_elements_range cf_draw3d_stats
 */
CF_API void CF_CALL cf_draw3d_mesh_range(CF_Mesh mesh, int first_element, int element_count);

/**
 * @function cf_draw3d_sprite
 * @category draw3d
 * @brief    Submits a sprite as a textured quad at a 3d position, oriented by the transform stack.
 * @param    sprite    The sprite. Its current frame supplies the image, exactly like
 *                     `cf_draw3d_push_texture`; animated sprites animate in the world.
 * @param    position  The world position of the quad's center.
 * @remarks  This is sugar over `cf_draw3d_mesh`: a shared unit quad in the xy plane (facing +z
 *           under an identity transform), sized `sprite->w` by `sprite->h` pixels times
 *           `sprite->scale` (set the scale to your world-units-per-pixel), composed onto the
 *           current transform stack, and submitted with the sprite pushed as its texture. Rotate
 *           it like anything else -- `cf_draw3d_rotate` before the call orients the quad, which
 *           is the tool for decals, standees, and paper-doll planes. Everything about mesh
 *           submission applies: your pushed shader receives the image via the contract's
 *           `in_uv`/`in_uv_rect` lanes, consecutive sprites coalesce into one instanced draw,
 *           and many different images share that draw through the texture atlas.
 *
 *           For quads that automatically face the camera, use `cf_draw3d_billboard`.
 * @related  cf_draw3d_billboard cf_draw3d_mesh cf_draw3d_push_texture CF_Sprite cf_draw3d_push_shader
 */
CF_API void CF_CALL cf_draw3d_sprite(const CF_Sprite* sprite, CF_V3 position);

/**
 * @function cf_draw3d_billboard
 * @category draw3d
 * @brief    Submits a sprite as a camera-facing quad at a 3d position -- a billboard.
 * @param    sprite    The sprite whose current frame supplies the image.
 * @param    position  The world position of the quad's center.
 * @remarks  Like `cf_draw3d_sprite`, but the quad is aimed with the current view's camera axes
 *           so it always faces the screen -- particles, pickups, impostors. The facing is full
 *           (spherical); for cylindrical billboards that pivot around y without tilting back
 *           (trees), build the basis yourself -- see the billboards sample, it is a handful of
 *           lines.
 * @related  cf_draw3d_sprite cf_draw3d_mesh cf_draw3d_push_texture CF_Sprite cf_draw3d_push_view
 */
CF_API void CF_CALL cf_draw3d_billboard(const CF_Sprite* sprite, CF_V3 position);

//--------------------------------------------------------------------------------------------------
// Shapes. The 3d analog of `cute_draw.h`'s shape drawing: debug lines, gizmos, and simple solid
// primitives with no shader or mesh setup required. Strokes (lines, circles, arcs) render as
// anti-aliased signed-distance ribbons -- smooth edges at any zoom without MSAA, and strokes
// thinner than a pixel fade out instead of shimmering. Solids (cube, sphere, cone, torus) draw
// with a simple built-in hemisphere light. All shapes respect the transform stack, the color
// stack below, layers, and draw list recording, and consecutive strokes batch into a single
// instanced draw.
//
// The transform stack scales shape scalars, not just positions: pushing a 2x scale doubles a
// circle's radius and a line's thickness the same way it doubles a sphere. (Under non-uniform
// scale a camera-facing ribbon has no single correct width, so the largest axis wins.) Thickness
// can instead be read as screen pixels -- see `cf_draw3d_push_stroke_pixels`, the mode gizmos and
// debug overlays usually want.
//
// Strokes compose their render state on top of whatever you push, forcing only what the
// technique itself needs. The depth *test*, target write masks, and stencil pass through
// untouched; depth writes are off unless `cf_draw3d_push_stroke_depth_write` turns them on; and
// the blend is premultiplied "over" only while the pushed state still carries the default
// opaque-replace blend. Push a real blend and it wins -- `CF_BLEND_OP_MAX` for glows that must
// never darken what they overlap, additive for light accumulation, and so on.

/**
 * @function cf_draw3d_push_color
 * @category draw3d
 * @brief    Pushes a color for subsequent 3d shape drawing.
 * @param    c  The color.
 * @remarks  Applies to all `cf_draw3d_` shape functions (lines, circles, solids). Defaults to white.
 * @related  cf_draw3d_pop_color cf_draw3d_peek_color cf_draw3d_line cf_draw3d_cube
 */
CF_API void CF_CALL cf_draw3d_push_color(CF_Color c);

/**
 * @function cf_draw3d_pop_color
 * @category draw3d
 * @brief    Pops and returns the last color pushed by `cf_draw3d_push_color`.
 * @related  cf_draw3d_push_color cf_draw3d_peek_color
 */
CF_API CF_Color CF_CALL cf_draw3d_pop_color(void);

/**
 * @function cf_draw3d_peek_color
 * @category draw3d
 * @brief    Returns the current 3d shape color without popping it.
 * @related  cf_draw3d_push_color cf_draw3d_pop_color
 */
CF_API CF_Color CF_CALL cf_draw3d_peek_color(void);

/**
 * @function cf_draw3d_push_dash
 * @category draw3d
 * @brief    Pushes a dash pattern for subsequent 3d stroke drawing.
 * @param    on_length   Length of each dash in world units.
 * @param    off_length  Length of each gap in world units.
 * @param    phase       Offset into the pattern in world units -- animate for marching ants.
 * @remarks  Applies to strokes (lines, polylines, circles, arcs, wire boxes); solids ignore it.
 *           Each dash gets round caps and the same local anti-aliasing and thin-stroke fading as
 *           solid strokes. An `on_length` of zero (the default) draws solid strokes. On full
 *           circles the pattern wraps at the seam unless `on_length + off_length` divides the
 *           circumference evenly.
 * @related  cf_draw3d_pop_dash cf_draw3d_peek_dash cf_draw3d_line cf_draw3d_circle cf_draw3d_arc
 */
CF_API void CF_CALL cf_draw3d_push_dash(float on_length, float off_length, float phase);

/**
 * @function cf_draw3d_pop_dash
 * @category draw3d
 * @brief    Pops and returns the last dash pattern pushed by `cf_draw3d_push_dash`.
 * @remarks  The returned vector holds x: on length, y: off length, z: phase.
 * @related  cf_draw3d_push_dash cf_draw3d_peek_dash
 */
CF_API CF_V3 CF_CALL cf_draw3d_pop_dash(void);

/**
 * @function cf_draw3d_peek_dash
 * @category draw3d
 * @brief    Returns the current dash pattern without popping it.
 * @remarks  The returned vector holds x: on length, y: off length, z: phase.
 * @related  cf_draw3d_push_dash cf_draw3d_pop_dash
 */
CF_API CF_V3 CF_CALL cf_draw3d_peek_dash(void);

/**
 * @function cf_draw3d_push_outline
 * @category draw3d
 * @brief    Draws an outline around subsequent 3d strokes.
 * @param    color  The outline color.
 * @param    width  Outline width in world units. Zero (the default) disables it.
 * @remarks  The 3d twin of `cf_draw_push_outline`: a band hugging the stroke's edge, drawn under
 *           the stroke itself, straight out of the same signed distance the stroke already
 *           evaluates -- no second pass and no extra geometry. Ideal for making a selected
 *           wireframe or gizmo read against a busy scene. Applies to strokes (lines, polylines,
 *           circles, arcs, wire boxes); solids ignore it. Unlike the 2d effect stack these ride
 *           uniforms rather than instance lanes, so strokes sharing effects still coalesce into
 *           one draw while changing effects splits the batch.
 * @related  cf_draw3d_pop_outline cf_draw3d_push_glow cf_draw_push_outline
 */
CF_API void CF_CALL cf_draw3d_push_outline(CF_Color color, float width);

/**
 * @function cf_draw3d_pop_outline
 * @category draw3d
 * @brief    Pops the last outline pushed by `cf_draw3d_push_outline`.
 * @related  cf_draw3d_push_outline cf_draw3d_push_glow
 */
CF_API void CF_CALL cf_draw3d_pop_outline(void);

/**
 * @function cf_draw3d_push_glow
 * @category draw3d
 * @brief    Draws a glow around subsequent 3d strokes.
 * @param    color   The glow color. Its alpha scales the effect's strength.
 * @param    radius  How far the glow reaches past the stroke, in world units. Zero (the default)
 *                   disables it.
 * @remarks  A smooth falloff radiating from the stroke's edge, evaluated analytically from its
 *           signed distance -- exact at any distance, no blur pass and no render target. The
 *           camera-facing ribbon grows automatically to fit the radius. Applies to strokes only;
 *           solids ignore it. Pair with an additive blend in the pushed render state for neon.
 * @related  cf_draw3d_pop_glow cf_draw3d_push_outline cf_draw_push_glow
 */
CF_API void CF_CALL cf_draw3d_push_glow(CF_Color color, float radius);

/**
 * @function cf_draw3d_pop_glow
 * @category draw3d
 * @brief    Pops the last glow pushed by `cf_draw3d_push_glow`.
 * @related  cf_draw3d_push_glow cf_draw3d_push_outline
 */
CF_API void CF_CALL cf_draw3d_pop_glow(void);

/**
 * @function cf_draw3d_push_stroke_pixels
 * @category draw3d
 * @brief    Switches stroke thickness between world units and screen pixels.
 * @param    screen_space  True to read thickness as pixels, false (the default) as world units.
 * @remarks  World-unit strokes are geometry: they get thinner with distance like everything else,
 *           which is what a wireframe or a drawn-in-the-world annotation wants. Screen-space
 *           strokes hold a constant on-screen width at any distance -- what editor gizmos,
 *           selection outlines, and debug overlays want. Thickness passed to `cf_draw3d_line` and
 *           friends is interpreted per this stack; shape *positions* and circle/arc radii stay in
 *           world units either way, since a shape's size is geometry and only its stroke width is
 *           a screen-space choice. Local anti-aliasing and sub-pixel fading work identically in
 *           both modes.
 * @related  cf_draw3d_pop_stroke_pixels cf_draw3d_peek_stroke_pixels cf_draw3d_line cf_draw3d_circle
 */
CF_API void CF_CALL cf_draw3d_push_stroke_pixels(bool screen_space);

/**
 * @function cf_draw3d_pop_stroke_pixels
 * @category draw3d
 * @brief    Pops and returns the last stroke sizing mode.
 * @related  cf_draw3d_push_stroke_pixels cf_draw3d_peek_stroke_pixels
 */
CF_API bool CF_CALL cf_draw3d_pop_stroke_pixels(void);

/**
 * @function cf_draw3d_peek_stroke_pixels
 * @category draw3d
 * @brief    Returns the current stroke sizing mode.
 * @related  cf_draw3d_push_stroke_pixels cf_draw3d_pop_stroke_pixels
 */
CF_API bool CF_CALL cf_draw3d_peek_stroke_pixels(void);

/**
 * @function cf_draw3d_push_stroke_depth_write
 * @category draw3d
 * @brief    Lets subsequent strokes write to the depth buffer, so they occlude each other.
 * @param    depth_write  True to write depth, false (the default) to only test against it.
 * @remarks  Strokes are anti-aliased ribbons, so by default they test depth but never write it:
 *           an AA fringe that owned depth would punch faint halos into whatever draws behind it
 *           later. That default also means strokes cannot occlude *each other* -- a wireframe box
 *           shows its back edges drawn over its front ones. Push this to trade the halo risk for
 *           self-occlusion, which is what an opaque wireframe usually wants. Only the stroke's
 *           solid core writes depth (fringe fragments are discarded), so the trade is small.
 *
 *           Effects never write depth, only cores do, and the multi-segment helpers
 *           (`cf_draw3d_box_wire`, `cf_draw3d_polyline`, `cf_draw3d_axes`) draw every segment's
 *           effects before any segment's core -- so outlines and depth writes compose, and a
 *           corner never shows one edge's outline sitting on its neighbor's core. Hand-rolled
 *           multi-stroke shapes made of individual `cf_draw3d_line` calls don't get that
 *           separation, so their shared joints can still show it.
 * @related  cf_draw3d_pop_stroke_depth_write cf_draw3d_push_render_state cf_draw3d_box_wire
 */
CF_API void CF_CALL cf_draw3d_push_stroke_depth_write(bool depth_write);

/**
 * @function cf_draw3d_pop_stroke_depth_write
 * @category draw3d
 * @brief    Pops and returns the last stroke depth-write mode.
 * @related  cf_draw3d_push_stroke_depth_write cf_draw3d_peek_stroke_depth_write
 */
CF_API bool CF_CALL cf_draw3d_pop_stroke_depth_write(void);

/**
 * @function cf_draw3d_peek_stroke_depth_write
 * @category draw3d
 * @brief    Returns the current stroke depth-write mode.
 * @related  cf_draw3d_push_stroke_depth_write cf_draw3d_pop_stroke_depth_write
 */
CF_API bool CF_CALL cf_draw3d_peek_stroke_depth_write(void);

/**
 * @function cf_make_draw3d_shape_shader
 * @category draw3d
 * @brief    Compiles a user shape shader from a file, for customizing built-in 3d shape rendering.
 * @param    virtual_path  A virtual path to the shader snippet (see the Virtual File System topic).
 * @remarks  The 3d analog of the 2d draw shader contract: your snippet defines
 *
 *           ```glsl
 *           vec4 shader(vec4 color, ShapeParams params)
 *           ```
 *
 *           which post-processes the built-in result of every shape drawn while the shader is
 *           pushed (`cf_draw3d_push_shader`). Strokes arrive premultiplied with local AA and
 *           thin-line fading already applied -- scale the whole vec4 or lerp with
 *           `mix(color, tint * color.a, t)` to stay premultiplied-safe; solids arrive
 *           hemisphere-lit with straight alpha. `ShapeParams` provides:
 *
 *           ```glsl
 *           struct ShapeParams
 *           {
 *               vec3 world_pos;   // Fragment world position on the stroke ribbon / solid surface.
 *               vec3 eye;         // Camera world position.
 *               vec3 normal;      // Solid surface normal; camera-facing for strokes.
 *               float sdf;        // Strokes: signed world-space distance to the stroke edge. Solids: 0.
 *               float fade;       // Thin-stroke fade already baked into color. Solids: 1.
 *               float kind;       // 0 line/arc body, 1 ring, 2 filled disc, 3 solid.
 *               vec4 attributes;  // cf_draw3d_push_mesh_attributes, captured per shape.
 *           };
 *           ```
 *
 *           Declare samplers at `set = 2` and your uniform block at `set = 3, binding = 1`
 *           (binding 0 belongs to the built-ins), named `shd_uniforms` -- exactly like 2d draw
 *           shaders. Values flow through `cf_draw3d_set_uniform` and `cf_draw3d_set_texture` as
 *           usual. Destroy the returned shader with `cf_destroy_shader`. Snippets under
 *           `cf_shader_directory` hot-reload in place while you edit, exactly like 2d draw
 *           shaders.
 * @related  cf_make_draw3d_shape_shader_from_source cf_draw3d_push_shader cf_draw3d_set_uniform cf_draw3d_push_mesh_attributes
 */
CF_API CF_Shader CF_CALL cf_make_draw3d_shape_shader(const char* virtual_path);

/**
 * @function cf_make_draw3d_shape_shader_from_source
 * @category draw3d
 * @brief    Compiles a user shape shader from an in-memory snippet.
 * @param    src  The snippet source defining `vec4 shader(vec4 color, ShapeParams params)`.
 * @remarks  See `cf_make_draw3d_shape_shader` for the full contract.
 * @related  cf_make_draw3d_shape_shader cf_draw3d_push_shader
 */
CF_API CF_Shader CF_CALL cf_make_draw3d_shape_shader_from_source(const char* src);

/**
 * @function cf_draw3d_line
 * @category draw3d
 * @brief    Draws an anti-aliased 3d line segment with round caps.
 * @param    p0         The first endpoint.
 * @param    p1         The second endpoint.
 * @param    thickness  The line's width in world units.
 * @remarks  Thickness is measured in world units, so lines get thinner with distance under a
 *           perspective camera; lines thinner than a pixel fade out gracefully rather than
 *           aliasing. Endpoints respect the transform stack (`cf_draw3d_push`).
 * @related  cf_draw3d_line2 cf_draw3d_polyline cf_draw3d_arrow cf_draw3d_push_color cf_draw3d_box_wire
 */
CF_API void CF_CALL cf_draw3d_line(CF_V3 p0, CF_V3 p1, float thickness);

/**
 * @function cf_draw3d_line2
 * @category draw3d
 * @brief    Draws an anti-aliased 3d line segment with a different thickness at each end.
 * @param    p0          The first endpoint.
 * @param    p1          The second endpoint.
 * @param    thickness0  Width at `p0` in world units.
 * @param    thickness1  Width at `p1` in world units.
 * @related  cf_draw3d_line cf_draw3d_polyline cf_draw3d_arrow
 */
CF_API void CF_CALL cf_draw3d_line2(CF_V3 p0, CF_V3 p1, float thickness0, float thickness1);

/**
 * @function cf_draw3d_polyline
 * @category draw3d
 * @brief    Draws connected anti-aliased line segments through a list of points.
 * @param    points     Array of points.
 * @param    count      Number of points.
 * @param    thickness  The line's width in world units.
 * @param    loop       True to connect the last point back to the first.
 * @remarks  Segments join with round caps, which look correct at any join angle.
 * @related  cf_draw3d_line cf_draw3d_box_wire cf_draw3d_push_color
 */
CF_API void CF_CALL cf_draw3d_polyline(const CF_V3* points, int count, float thickness, bool loop);

/**
 * @function cf_draw3d_arrow
 * @category draw3d
 * @brief    Draws a line from `a` to `b` capped with a solid cone arrowhead at `b`.
 * @param    a            The tail.
 * @param    b            The tip.
 * @param    thickness    The shaft's width in world units.
 * @param    arrow_width  The arrowhead's base radius in world units.
 * @related  cf_draw3d_line cf_draw3d_axes cf_draw3d_cone
 */
CF_API void CF_CALL cf_draw3d_arrow(CF_V3 a, CF_V3 b, float thickness, float arrow_width);

/**
 * @function cf_draw3d_circle
 * @category draw3d
 * @brief    Draws an anti-aliased circle outline in 3d.
 * @param    center     The circle's center.
 * @param    normal     The plane's normal (need not be normalized).
 * @param    radius     The circle's radius.
 * @param    thickness  The stroke's width in world units.
 * @related  cf_draw3d_circle_fill cf_draw3d_arc cf_draw3d_sphere
 */
CF_API void CF_CALL cf_draw3d_circle(CF_V3 center, CF_V3 normal, float radius, float thickness);

/**
 * @function cf_draw3d_circle_fill
 * @category draw3d
 * @brief    Draws an anti-aliased filled disc in 3d.
 * @param    center  The disc's center.
 * @param    normal  The plane's normal (need not be normalized).
 * @param    radius  The disc's radius.
 * @related  cf_draw3d_circle cf_draw3d_arc
 */
CF_API void CF_CALL cf_draw3d_circle_fill(CF_V3 center, CF_V3 normal, float radius);

/**
 * @function cf_draw3d_arc
 * @category draw3d
 * @brief    Draws an anti-aliased arc (a partial circle outline) with round ends.
 * @param    center     The arc's center.
 * @param    normal     The plane's normal (need not be normalized).
 * @param    radius     The arc's radius.
 * @param    angle0     Starting angle in radians, measured in the arc's plane.
 * @param    sweep      Radians swept counter-clockwise from `angle0`. `CF_TAU` or more draws a full circle.
 * @param    thickness  The stroke's width in world units.
 * @related  cf_draw3d_circle cf_draw3d_circle_fill
 */
CF_API void CF_CALL cf_draw3d_arc(CF_V3 center, CF_V3 normal, float radius, float angle0, float sweep, float thickness);

/**
 * @function cf_draw3d_box_wire
 * @category draw3d
 * @brief    Draws the twelve edges of an axis-aligned box as anti-aliased lines.
 * @param    center        The box's center.
 * @param    half_extents  Half the box's size along each axis.
 * @param    thickness     The stroke's width in world units.
 * @remarks  Axis-aligned in local space -- push a rotation onto the transform stack for an
 *           oriented box. The classic bounding-box debug visualization.
 * @related  cf_draw3d_cube cf_draw3d_line cf_draw3d_push
 */
CF_API void CF_CALL cf_draw3d_box_wire(CF_V3 center, CF_V3 half_extents, float thickness);

/**
 * @function cf_draw3d_axes
 * @category draw3d
 * @brief    Draws x/y/z axis lines from the local origin, colored red/green/blue.
 * @param    length     Each axis line's length in world units.
 * @param    thickness  The stroke's width in world units.
 * @remarks  Respects the transform stack, so pushing an object's transform first draws that
 *           object's local frame -- the standard orientation gizmo.
 * @related  cf_draw3d_arrow cf_draw3d_line cf_draw3d_push
 */
CF_API void CF_CALL cf_draw3d_axes(float length, float thickness);

/**
 * @function cf_draw3d_cube
 * @category draw3d
 * @brief    Draws a solid box lit by a simple built-in hemisphere light.
 * @param    center        The box's center.
 * @param    half_extents  Half the box's size along each axis.
 * @remarks  Colored by `cf_draw3d_push_color` and drawn under the current render state (depth
 *           testing on by default). Push a rotation onto the transform stack for an oriented box.
 * @related  cf_draw3d_sphere cf_draw3d_cone cf_draw3d_torus cf_draw3d_box_wire cf_draw3d_push_color
 */
CF_API void CF_CALL cf_draw3d_cube(CF_V3 center, CF_V3 half_extents);

/**
 * @function cf_draw3d_sphere
 * @category draw3d
 * @brief    Draws a solid sphere lit by a simple built-in hemisphere light.
 * @param    center  The sphere's center.
 * @param    radius  The sphere's radius.
 * @related  cf_draw3d_cube cf_draw3d_cone cf_draw3d_torus cf_draw3d_circle cf_draw3d_push_color
 */
CF_API void CF_CALL cf_draw3d_sphere(CF_V3 center, float radius);

/**
 * @function cf_draw3d_cone
 * @category draw3d
 * @brief    Draws a solid cone lit by a simple built-in hemisphere light.
 * @param    base    The center of the cone's circular base.
 * @param    tip     The cone's apex.
 * @param    radius  The base's radius.
 * @related  cf_draw3d_cube cf_draw3d_sphere cf_draw3d_torus cf_draw3d_arrow cf_draw3d_push_color
 */
CF_API void CF_CALL cf_draw3d_cone(CF_V3 base, CF_V3 tip, float radius);

/**
 * @function cf_draw3d_cylinder
 * @category draw3d
 * @brief    Draws a solid cylinder lit by a simple built-in hemisphere light.
 * @param    a       The center of one end cap.
 * @param    b       The center of the other end cap.
 * @param    radius  The cylinder's radius.
 * @related  cf_draw3d_capsule cf_draw3d_cube cf_draw3d_sphere cf_draw3d_cone cf_draw3d_push_color
 */
CF_API void CF_CALL cf_draw3d_cylinder(CF_V3 a, CF_V3 b, float radius);

/**
 * @function cf_draw3d_capsule
 * @category draw3d
 * @brief    Draws a solid capsule lit by a simple built-in hemisphere light.
 * @param    a       The center of one hemispherical cap.
 * @param    b       The center of the other cap.
 * @param    radius  The capsule's radius.
 * @remarks  The debug shape for character controllers and swept spheres. Degenerates to a
 *           sphere when `a` equals `b`. Composed from three sub-mesh records (side + two
 *           caps) that coalesce with every other interleaved solid, so drawing one costs
 *           the same batch as any other shape.
 * @related  cf_draw3d_cylinder cf_draw3d_sphere cf_draw3d_cube cf_draw3d_cone cf_draw3d_push_color
 */
CF_API void CF_CALL cf_draw3d_capsule(CF_V3 a, CF_V3 b, float radius);

/**
 * @function cf_draw3d_torus
 * @category draw3d
 * @brief    Draws a solid torus lit by a simple built-in hemisphere light.
 * @param    center       The torus's center.
 * @param    normal       The normal of the torus's plane (need not be normalized).
 * @param    radius       The major radius, from the center to the middle of the tube.
 * @param    tube_radius  The tube's radius.
 * @related  cf_draw3d_cube cf_draw3d_sphere cf_draw3d_cone cf_draw3d_circle cf_draw3d_push_color
 */
CF_API void CF_CALL cf_draw3d_torus(CF_V3 center, CF_V3 normal, float radius, float tube_radius);

//--------------------------------------------------------------------------------------------------
// Projection helpers. These bridge the 3d camera stacks and the 2d draw API's world space --
// the space `cf_draw_text` draws in and `cf_screen_to_world` converts mouse coordinates into.
// Together they cover the two classic needs: labels over 3d objects, and mouse picking.

/**
 * @function cf_draw3d_project
 * @category draw3d
 * @brief    Projects a 3d world position to the 2d draw API's world space.
 * @param    world_position  The 3d position, in the space `cf_draw3d_mesh` submissions use.
 * @return   Returns xy in 2d draw coordinates and z as the depth in [0, 1]. A negative z means
 *           the position is behind the camera and xy are unusable.
 * @remarks  Uses the current 3d projection/view stacks and the current 2d camera, so drawing 2d
 *           at the returned xy lands on the same pixel the 3d position rasterizes to -- health
 *           bars, nameplates, and damage numbers over 3d objects:
 *
 *           ```c
 *           CF_V3 p = cf_draw3d_project(enemy_position);
 *           if (p.z >= 0) cf_draw_text("orc", cf_v2(p.x, p.y + 12.0f), -1);
 *           ```
 * @related  cf_draw3d_unproject cf_draw3d_push_projection cf_draw3d_push_view cf_world_to_screen
 */
CF_API CF_V3 CF_CALL cf_draw3d_project(CF_V3 world_position);

/**
 * @function cf_draw3d_unproject
 * @category draw3d
 * @brief    Turns a 2d point into a 3d ray under the current camera stacks.
 * @param    point          A point in the 2d draw API's world space.
 * @param    origin_out     The ray's origin on the near plane. Can be `NULL`.
 * @param    direction_out  The ray's normalized direction. Can be `NULL`.
 * @remarks  The front half of mouse picking -- convert the mouse with `cf_screen_to_world`, then
 *           cast the returned ray against your own scene representation:
 *
 *           ```c
 *           CF_V3 origin, dir;
 *           cf_draw3d_unproject(cf_screen_to_world(cf_v2(cf_mouse_x(), cf_mouse_y())), &origin, &dir);
 *           ```
 *
 *           Works for perspective and orthographic projections alike (orthographic rays share a
 *           direction but differ in origin).
 * @related  cf_draw3d_project cf_screen_to_world cf_draw3d_push_projection cf_draw3d_push_view
 */
CF_API void CF_CALL cf_draw3d_unproject(CF_V2 point, CF_V3* origin_out, CF_V3* direction_out);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

CF_INLINE void draw3d_push_projection(CF_M4x4 projection) { cf_draw3d_push_projection(projection); }
CF_INLINE CF_M4x4 draw3d_pop_projection() { return cf_draw3d_pop_projection(); }
CF_INLINE CF_M4x4 draw3d_peek_projection() { return cf_draw3d_peek_projection(); }
CF_INLINE void draw3d_push_view(CF_M4x4 view) { cf_draw3d_push_view(view); }
CF_INLINE CF_M4x4 draw3d_pop_view() { return cf_draw3d_pop_view(); }
CF_INLINE CF_M4x4 draw3d_peek_view() { return cf_draw3d_peek_view(); }

CF_INLINE void draw3d_push() { cf_draw3d_push(); }
CF_INLINE void draw3d_pop() { cf_draw3d_pop(); }
CF_INLINE void draw3d_translate(CF_V3 t) { cf_draw3d_translate(t); }
CF_INLINE void draw3d_rotate(CF_Quat q) { cf_draw3d_rotate(q); }
CF_INLINE void draw3d_scale(CF_V3 s) { cf_draw3d_scale(s); }
CF_INLINE void draw3d_transform(CF_M4x4 m) { cf_draw3d_transform(m); }
CF_INLINE CF_M4x4 draw3d_peek_transform() { return cf_draw3d_peek_transform(); }
CF_INLINE CF_V3 draw3d_mul(CF_V3 p) { return cf_draw3d_mul(p); }

CF_INLINE void draw3d_push_shader(CF_Shader shader) { cf_draw3d_push_shader(shader); }
CF_INLINE CF_Shader draw3d_pop_shader() { return cf_draw3d_pop_shader(); }
CF_INLINE CF_Shader draw3d_peek_shader() { return cf_draw3d_peek_shader(); }
CF_INLINE void draw3d_push_render_state(CF_RenderState render_state) { cf_draw3d_push_render_state(render_state); }
CF_INLINE CF_RenderState draw3d_pop_render_state() { return cf_draw3d_pop_render_state(); }
CF_INLINE CF_RenderState draw3d_peek_render_state() { return cf_draw3d_peek_render_state(); }

CF_INLINE void draw3d_set_uniform(const char* name, void* data, CF_UniformType type, int array_length) { cf_draw3d_set_uniform(name, data, type, array_length); }
CF_INLINE void draw3d_set_uniform(const char* name, int val) { cf_draw3d_set_uniform_int(name, val); }
CF_INLINE void draw3d_set_uniform(const char* name, float val) { cf_draw3d_set_uniform_float(name, val); }
CF_INLINE void draw3d_set_uniform(const char* name, CF_V2 val) { cf_draw3d_set_uniform_v2(name, val); }
CF_INLINE void draw3d_set_uniform(const char* name, CF_V3 val) { cf_draw3d_set_uniform_v3(name, val); }
CF_INLINE void draw3d_set_uniform(const char* name, CF_M4x4 val) { cf_draw3d_set_uniform_m4(name, val); }
CF_INLINE void draw3d_set_uniform(const char* name, CF_Color val) { cf_draw3d_set_uniform_color(name, val); }
CF_INLINE void draw3d_set_texture(const char* name, CF_Texture texture) { cf_draw3d_set_texture(name, texture); }
CF_INLINE void draw3d_set_vs_storage_buffers(CF_StorageBuffer* buffers, int count) { cf_draw3d_set_vs_storage_buffers(buffers, count); }

CF_INLINE void draw3d_push_texture(const CF_Sprite* sprite) { cf_draw3d_push_texture(sprite); }
CF_INLINE void draw3d_pop_texture() { cf_draw3d_pop_texture(); }
CF_INLINE const CF_Sprite* draw3d_peek_texture() { return cf_draw3d_peek_texture(); }
CF_INLINE void draw3d_push_mesh_attributes(CF_V4 attributes) { cf_draw3d_push_mesh_attributes(attributes); }
CF_INLINE CF_V4 draw3d_pop_mesh_attributes() { return cf_draw3d_pop_mesh_attributes(); }
CF_INLINE CF_V4 draw3d_peek_mesh_attributes() { return cf_draw3d_peek_mesh_attributes(); }

CF_INLINE void draw3d_mesh(CF_Mesh mesh) { cf_draw3d_mesh(mesh); }
CF_INLINE void draw3d_mesh_range(CF_Mesh mesh, int first_element, int element_count) { cf_draw3d_mesh_range(mesh, first_element, element_count); }
CF_INLINE void draw3d_sprite(const CF_Sprite* sprite, CF_V3 position) { cf_draw3d_sprite(sprite, position); }
CF_INLINE void draw3d_billboard(const CF_Sprite* sprite, CF_V3 position) { cf_draw3d_billboard(sprite, position); }
CF_INLINE void draw3d_push_color(CF_Color c) { cf_draw3d_push_color(c); }
CF_INLINE CF_Color draw3d_pop_color() { return cf_draw3d_pop_color(); }
CF_INLINE CF_Color draw3d_peek_color() { return cf_draw3d_peek_color(); }
CF_INLINE void draw3d_push_mesh_attributes2(CF_V4 attributes) { cf_draw3d_push_mesh_attributes2(attributes); }
CF_INLINE CF_V4 draw3d_pop_mesh_attributes2() { return cf_draw3d_pop_mesh_attributes2(); }
CF_INLINE CF_V4 draw3d_peek_mesh_attributes2() { return cf_draw3d_peek_mesh_attributes2(); }
CF_INLINE void draw3d_push_outline(CF_Color color, float width) { cf_draw3d_push_outline(color, width); }
CF_INLINE void draw3d_pop_outline() { cf_draw3d_pop_outline(); }
CF_INLINE void draw3d_push_glow(CF_Color color, float radius) { cf_draw3d_push_glow(color, radius); }
CF_INLINE void draw3d_pop_glow() { cf_draw3d_pop_glow(); }
CF_INLINE void draw3d_push_stroke_depth_write(bool depth_write) { cf_draw3d_push_stroke_depth_write(depth_write); }
CF_INLINE bool draw3d_pop_stroke_depth_write() { return cf_draw3d_pop_stroke_depth_write(); }
CF_INLINE bool draw3d_peek_stroke_depth_write() { return cf_draw3d_peek_stroke_depth_write(); }
CF_INLINE void draw3d_push_stroke_pixels(bool screen_space) { cf_draw3d_push_stroke_pixels(screen_space); }
CF_INLINE bool draw3d_pop_stroke_pixels() { return cf_draw3d_pop_stroke_pixels(); }
CF_INLINE bool draw3d_peek_stroke_pixels() { return cf_draw3d_peek_stroke_pixels(); }
CF_INLINE void draw3d_push_dash(float on_length, float off_length, float phase) { cf_draw3d_push_dash(on_length, off_length, phase); }
CF_INLINE CF_V3 draw3d_pop_dash() { return cf_draw3d_pop_dash(); }
CF_INLINE CF_V3 draw3d_peek_dash() { return cf_draw3d_peek_dash(); }
CF_INLINE CF_Shader make_draw3d_shape_shader(const char* virtual_path) { return cf_make_draw3d_shape_shader(virtual_path); }
CF_INLINE CF_Shader make_draw3d_shape_shader_from_source(const char* src) { return cf_make_draw3d_shape_shader_from_source(src); }
CF_INLINE void draw3d_line(CF_V3 p0, CF_V3 p1, float thickness) { cf_draw3d_line(p0, p1, thickness); }
CF_INLINE void draw3d_line2(CF_V3 p0, CF_V3 p1, float thickness0, float thickness1) { cf_draw3d_line2(p0, p1, thickness0, thickness1); }
CF_INLINE void draw3d_polyline(const CF_V3* points, int count, float thickness, bool loop) { cf_draw3d_polyline(points, count, thickness, loop); }
CF_INLINE void draw3d_arrow(CF_V3 a, CF_V3 b, float thickness, float arrow_width) { cf_draw3d_arrow(a, b, thickness, arrow_width); }
CF_INLINE void draw3d_circle(CF_V3 center, CF_V3 normal, float radius, float thickness) { cf_draw3d_circle(center, normal, radius, thickness); }
CF_INLINE void draw3d_circle_fill(CF_V3 center, CF_V3 normal, float radius) { cf_draw3d_circle_fill(center, normal, radius); }
CF_INLINE void draw3d_arc(CF_V3 center, CF_V3 normal, float radius, float angle0, float sweep, float thickness) { cf_draw3d_arc(center, normal, radius, angle0, sweep, thickness); }
CF_INLINE void draw3d_box_wire(CF_V3 center, CF_V3 half_extents, float thickness) { cf_draw3d_box_wire(center, half_extents, thickness); }
CF_INLINE void draw3d_axes(float length, float thickness) { cf_draw3d_axes(length, thickness); }
CF_INLINE void draw3d_cube(CF_V3 center, CF_V3 half_extents) { cf_draw3d_cube(center, half_extents); }
CF_INLINE void draw3d_sphere(CF_V3 center, float radius) { cf_draw3d_sphere(center, radius); }
CF_INLINE void draw3d_cone(CF_V3 base, CF_V3 tip, float radius) { cf_draw3d_cone(base, tip, radius); }
CF_INLINE void draw3d_cylinder(CF_V3 a, CF_V3 b, float radius) { cf_draw3d_cylinder(a, b, radius); }
CF_INLINE void draw3d_capsule(CF_V3 a, CF_V3 b, float radius) { cf_draw3d_capsule(a, b, radius); }
CF_INLINE void draw3d_torus(CF_V3 center, CF_V3 normal, float radius, float tube_radius) { cf_draw3d_torus(center, normal, radius, tube_radius); }
CF_INLINE CF_V3 draw3d_project(CF_V3 world_position) { return cf_draw3d_project(world_position); }
CF_INLINE void draw3d_unproject(CF_V2 point, CF_V3* origin_out, CF_V3* direction_out) { cf_draw3d_unproject(point, origin_out, direction_out); }

}

#endif // CF_CPP

#endif // CF_DRAW3D_H
