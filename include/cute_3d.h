/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_3D_H
#define CF_3D_H

#include "cute_defines.h"
#include "cute_math3d.h"
#include "cute_graphics.h"
#include "cute_color.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Vertex3D
 * @category graphics
 * @brief    The standard 3d vertex: position, normal and texture coordinate.
 * @remarks  `cf_make_mesh` can describe any vertex layout you like, and for anything unusual you
 *           should keep using it. This struct exists so the common case has one blessed layout that
 *           the built-in shader from `cf_make_lit_shader_3d` already understands, and so that meshes
 *           from different sources interoperate.
 *
 *           The matching vertex shader inputs are:
 *
 *           ```glsl
 *           layout (location = 0) in vec3 in_pos;
 *           layout (location = 1) in vec3 in_normal;
 *           layout (location = 2) in vec2 in_uv;
 *           ```
 * @related  CF_Vertex3D cf_make_mesh_3d cf_make_lit_shader_3d CF_Mesh cf_make_mesh
 */
typedef struct CF_Vertex3D
{
	/* @member Object-space position. */
	CF_V3 pos;

	/* @member Object-space normal. Should be unit length. */
	CF_V3 normal;

	/* @member Texture coordinate. */
	CF_V2 uv;
} CF_Vertex3D;
// @end

/**
 * @function cf_make_mesh_3d
 * @category graphics
 * @brief    Creates a `CF_Mesh` from `CF_Vertex3D` data, with the vertex attributes filled in.
 * @param    vertices      The vertex data. Copied into the mesh.
 * @param    vertex_count  How many vertices.
 * @param    indices       32-bit indices, or `NULL` for a non-indexed mesh.
 * @param    index_count   How many indices. Ignored when `indices` is `NULL`.
 * @return   Returns a `CF_Mesh` ready for `cf_apply_mesh`. Destroy it with `cf_destroy_mesh`.
 * @remarks  A convenience wrapper over `cf_make_mesh` that supplies the stride, offsets and
 *           attribute names for `CF_Vertex3D`. To change the data later call
 *           `cf_mesh_update_vertex_data` and `cf_mesh_update_index_data` as usual -- the buffers are
 *           sized exactly to what you pass here, so re-uploading more vertices than the original
 *           count needs a new mesh.
 * @related  CF_Vertex3D CF_Mesh cf_make_mesh cf_destroy_mesh cf_apply_mesh cf_make_lit_shader_3d
 */
CF_API CF_Mesh CF_CALL cf_make_mesh_3d(const CF_Vertex3D* vertices, int vertex_count, const uint32_t* indices, int index_count);

/**
 * @function cf_make_lit_shader_3d
 * @category graphics
 * @brief    Creates the built-in 3d shader: an MVP transform, one directional light, one albedo texture.
 * @return   Returns a `CF_Shader` for use with `cf_apply_shader`. Destroy it with `cf_destroy_shader`.
 * @remarks  This is the "textured mesh under a directional light" base case, so that getting pixels
 *           on screen does not require writing a shader first. It expects the `CF_Vertex3D` layout,
 *           so pair it with `cf_make_mesh_3d`. Set its uniforms with `cf_lit_shader_3d_set_transform`,
 *           `cf_lit_shader_3d_set_light` and `cf_lit_shader_3d_set_albedo`.
 *
 *           Shading is Lambert plus a flat ambient term -- deliberately plain. Anything beyond that
 *           (toon ramps, normal maps, multiple lights) means writing your own `cf_make_shader`; this
 *           exists so you are not forced to do that on day one.
 *
 *           Remember that depth testing needs both `cf_render_state_3d_defaults` on the material and
 *           a canvas created with `depth_stencil_enable` set to true.
 * @related  cf_make_mesh_3d CF_Vertex3D cf_lit_shader_3d_set_transform cf_lit_shader_3d_set_light cf_lit_shader_3d_set_albedo cf_render_state_3d_defaults
 */
CF_API CF_Shader CF_CALL cf_make_lit_shader_3d(void);

/**
 * @function cf_lit_shader_3d_set_transform
 * @category graphics
 * @brief    Sets the transform uniforms for `cf_make_lit_shader_3d`.
 * @param    material  The material to set uniforms on.
 * @param    mvp       The full model-view-projection matrix.
 * @param    model     The model matrix alone, used to light in world space.
 * @remarks  The normal matrix is derived from `model` via `cf_m4_normal_matrix`, so non-uniform scale
 *           is handled correctly without you having to think about it.
 * @related  cf_make_lit_shader_3d cf_lit_shader_3d_set_light cf_m4_normal_matrix cf_perspective cf_look_at
 */
CF_API void CF_CALL cf_lit_shader_3d_set_transform(CF_Material material, CF_M4x4 mvp, CF_M4x4 model);

/**
 * @function cf_lit_shader_3d_set_light
 * @category graphics
 * @brief    Sets the lighting uniforms for `cf_make_lit_shader_3d`.
 * @param    material   The material to set uniforms on.
 * @param    direction  The direction the light travels, in world space. Normalized for you.
 * @param    color      The light's color.
 * @param    ambient    A flat term added everywhere, so surfaces facing away are not pure black.
 * @remarks  `direction` is the direction light *travels*, not the direction toward the light: a sun
 *           overhead is `cf_v3(0, -1, 0)`.
 * @related  cf_make_lit_shader_3d cf_lit_shader_3d_set_transform cf_lit_shader_3d_set_albedo
 */
CF_API void CF_CALL cf_lit_shader_3d_set_light(CF_Material material, CF_V3 direction, CF_Color color, CF_Color ambient);

/**
 * @function cf_lit_shader_3d_set_albedo
 * @category graphics
 * @brief    Sets the albedo texture for `cf_make_lit_shader_3d`.
 * @param    material  The material to set the texture on.
 * @param    texture   The base-color texture, sampled with the vertex uv.
 * @remarks  The shader always samples a texture. For untextured geometry bind a 1x1 white texture
 *           and tint with the light color.
 * @related  cf_make_lit_shader_3d cf_lit_shader_3d_set_light cf_material_set_texture_fs CF_Texture
 */
CF_API void CF_CALL cf_lit_shader_3d_set_albedo(CF_Material material, CF_Texture texture);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

CF_INLINE CF_Mesh make_mesh_3d(const CF_Vertex3D* vertices, int vertex_count, const uint32_t* indices, int index_count) { return cf_make_mesh_3d(vertices, vertex_count, indices, index_count); }
CF_INLINE CF_Shader make_lit_shader_3d() { return cf_make_lit_shader_3d(); }
CF_INLINE void lit_shader_3d_set_transform(CF_Material material, CF_M4x4 mvp, CF_M4x4 model) { cf_lit_shader_3d_set_transform(material, mvp, model); }
CF_INLINE void lit_shader_3d_set_light(CF_Material material, CF_V3 direction, CF_Color color, CF_Color ambient) { cf_lit_shader_3d_set_light(material, direction, color, ambient); }
CF_INLINE void lit_shader_3d_set_albedo(CF_Material material, CF_Texture texture) { cf_lit_shader_3d_set_albedo(material, texture); }

}

#endif // CF_CPP

#endif // CF_3D_H
