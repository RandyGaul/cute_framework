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
 * @related  CF_Shader cf_make_shader cf_draw3d_push_shader cf_draw3d_pop_shader cf_draw3d_peek_shader cf_draw3d_mesh
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
 *           `cf_canvas_get_depth_stencil_target` for depth.
 * @related  cf_draw3d_set_texture cf_draw3d_set_uniform CF_Texture cf_canvas_get_target cf_canvas_get_depth_stencil_target
 */
CF_API void CF_CALL cf_draw3d_set_texture(const char* name, CF_Texture texture);

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
 *           `cf_draw_push_layer` orders them, and opaque meshes need no ordering at all beyond the
 *           depth test. Winding is counter-clockwise for front faces under the default render
 *           state. A mesh carrying its own instance buffer is drawn as-is with no reserved
 *           attributes bound -- see the escape hatch note at the top of this header.
 * @related  CF_Mesh cf_make_mesh cf_draw3d_push_shader cf_draw3d_transform cf_draw3d_push_mesh_attributes cf_render_to cf_make_draw_list
 */
CF_API void CF_CALL cf_draw3d_mesh(CF_Mesh mesh);

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

CF_INLINE void draw3d_push_texture(const CF_Sprite* sprite) { cf_draw3d_push_texture(sprite); }
CF_INLINE void draw3d_pop_texture() { cf_draw3d_pop_texture(); }
CF_INLINE const CF_Sprite* draw3d_peek_texture() { return cf_draw3d_peek_texture(); }
CF_INLINE void draw3d_push_mesh_attributes(CF_V4 attributes) { cf_draw3d_push_mesh_attributes(attributes); }
CF_INLINE CF_V4 draw3d_pop_mesh_attributes() { return cf_draw3d_pop_mesh_attributes(); }
CF_INLINE CF_V4 draw3d_peek_mesh_attributes() { return cf_draw3d_peek_mesh_attributes(); }

CF_INLINE void draw3d_mesh(CF_Mesh mesh) { cf_draw3d_mesh(mesh); }

}

#endif // CF_CPP

#endif // CF_DRAW3D_H
