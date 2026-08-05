# 3D Drawing

CF's 3D story is deliberately different from most engines: **CF gives you access, not a renderer**. There is no material system, no lighting model, no scene graph, and no assumptions about how you shade. Instead, [cute_draw3d.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw3d.h) owns exactly two things -- transforms and submission -- and you own every fragment. Shadow maps, g-buffers, toon pipelines, fog, skinning: all of these are *your shaders* composed out of CF's mechanisms, and the [samples](#the-samples) prove each one in under a few hundred lines.

If you've used the 2D draw API, you already know how this one feels. State is stack-based (`push`/`pop`/`peek`), submissions are immediate-mode, and meshes flow through the *same command stream* as sprites, shapes, and text -- `cf_draw_push_layer` orders 3D against 2D, `cf_render_to` flushes everything together, and `CF_DrawList` records 3D submissions just like 2D ones.

## Your First Mesh

Three ingredients: a mesh, a shader following the contract, and a camera.

```cpp
// A mesh is vertices you assemble yourself -- any layout you like.
CF_VertexAttribute attrs[2] = { };
attrs[0].name = "in_pos";    attrs[0].format = CF_VERTEX_FORMAT_FLOAT3; attrs[0].offset = 0;
attrs[1].name = "in_normal"; attrs[1].format = CF_VERTEX_FORMAT_FLOAT3; attrs[1].offset = 12;
CF_Mesh cube = cf_make_mesh(sizeof(verts), attrs, 2, sizeof(Vertex));
cf_mesh_update_vertex_data(cube, verts, vert_count);

CF_Shader shader = cf_make_shader_from_source(my_vs, my_fs);

// Each frame:
cf_draw3d_push_projection(cf_perspective(CF_PI / 3.0f, aspect, 0.1f, 100.0f));
cf_draw3d_push_view(cf_look_at(eye, target, cf_v3(0, 1, 0)));
cf_draw3d_push_shader(shader);

cf_draw3d_push();
cf_draw3d_translate(cf_v3(0, 1, 0));
cf_draw3d_rotate(cf_quat_from_axis_angle(cf_v3(0, 1, 0), turns));
cf_draw3d_mesh(cube);
cf_draw3d_pop();

cf_draw3d_pop_shader();
cf_draw3d_pop_view();
cf_draw3d_pop_projection();

cf_app_draw_onto_screen(true);
```

There is no default 3D shader, on purpose -- submitting a mesh without one pushed is an error. The contract below is the entire interface between your shader and this layer.

## The Shader Contract

Instancing in CF is automatic (more below), so per-mesh data rides in *instance-rate vertex attributes* rather than uniforms. Your vertex shader declares the reserved inputs it uses, alongside your own attributes, plus one required uniform member:

```glsl
// Your mesh's own attributes, bound by name as usual:
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

// Reserved, fed by CF per mesh submission (declare only what you use):
layout (location = 8)  in vec4 in_model0;          // model transform, affine row 0
layout (location = 9)  in vec4 in_model1;          // row 1
layout (location = 10) in vec4 in_model2;          // row 2
layout (location = 11) in vec4 in_uv_rect;         // cf_draw3d_push_texture's atlas sub-rect
layout (location = 12) in vec4 in_nmat0;           // normal matrix row 0
layout (location = 13) in vec4 in_nmat1;           // row 1
layout (location = 14) in vec4 in_nmat2;           // row 2
layout (location = 15) in vec4 in_mesh_attributes; // cf_draw3d_push_mesh_attributes

layout (set = 1, binding = 0) uniform uniform_block {
    mat4 u_view_projection; // Set by CF from the camera stacks.
    // ...your own vertex uniforms...
};

void main()
{
    vec4 p = vec4(in_pos, 1.0);
    vec3 world_pos = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));
    vec3 world_normal = normalize(vec3(dot(in_nmat0.xyz, in_normal),
                                       dot(in_nmat1.xyz, in_normal),
                                       dot(in_nmat2.xyz, in_normal)));
    gl_Position = u_view_projection * vec4(world_pos, 1.0);
}
```

Everything after `gl_Position` -- lighting, shadowing, whatever -- is yours. The fragment stage is entirely unconstrained.

## Instancing is Automatic

Consecutive submissions of the same mesh under the same shader, render state, textures, and uniforms coalesce into a single instanced draw. Drawing one rock 500 times in a loop is one draw call, the same way 500 sprites are. You never ask for instancing; you just draw.

Per-submission variety that does *not* split the draw: the transform, and one `vec4` of per-mesh data via `cf_draw3d_push_mesh_attributes` (delivered as `in_mesh_attributes` -- tint, random seed, animation phase, whatever your shading wants). Changing a *uniform* between submissions does split the draw, which is the lever for material variety within one shader.

A mesh that carries its own instance buffer (`cf_mesh_set_instance_buffer`) is the escape hatch: it draws as-is, no reserved attributes bound, instancing entirely yours.

### Geometry Arenas

The one thing automatic instancing can't fix on its own is *interleaving*: draw rock, tree, rock, tree and every submission splits the batch, because each needs a different mesh bound. The fix is to pack the small meshes into one `CF_Mesh` -- a geometry arena -- and submit sub-ranges:

```cpp
cf_draw3d_mesh_range(arena, rock_first_vertex, rock_vertex_count);
cf_draw3d_mesh_range(arena, tree_first_vertex, tree_vertex_count);
```

Now every submission shares the same mesh, so the whole interleaved stream coalesces into one command that binds everything once and issues one tight range draw per contiguous record. In the Debug bench this is the difference between 18 ms and 1.9 ms a frame for 2000 interleaved objects. Indexed arenas write their indices absolute into the shared vertex buffer (there's no base-vertex parameter, which is what keeps ranges portable to GLES3/web). `cf_draw3d_stats` tells you when interleaving is splitting your batches in the first place -- watch `splits`.

### Pull Instancing with Storage Buffers

When per-instance data outgrows the reserved attribute lanes -- bone palettes being the classic case -- bind storage buffers to the vertex stage and index them per instance:

```cpp
cf_draw3d_set_vs_storage_buffers(&bones, 1); // Captured per submission, like uniforms.
```

```glsl
layout (std430, set = 0, binding = 0) readonly buffer bones_buffer { vec4 u_bones[]; };
```

Buffer-block tails must be scalars or vectors, so a mat4 palette stores four `vec4` columns per bone and reassembles in the shader. A free `cf_draw3d_push_mesh_attributes` lane carries each submission's base offset into the shared buffer, so many characters on different animations still coalesce -- the `model3d` sample runs a five-fox pack on independent clips in three draws with this exact pattern.

## Uniforms and Textures

`cf_draw3d_set_uniform*` and `cf_draw3d_set_texture` are plain named state, captured with each submission -- the same idiom as the 2D API. Bind a previous pass by binding its canvas targets:

```cpp
cf_draw3d_set_texture("u_shadow", cf_canvas_get_depth_stencil_target(shadow_canvas));
cf_draw3d_set_uniform_m4("u_light_vp", light_vp);
```

One convention worth memorizing: when your shader maps clip-space positions to canvas uvs (shadow maps, screen-space reads), **v runs top-down** -- `uv = vec2(ndc.x, -ndc.y) * 0.5 + 0.5` on every backend.

## Shapes and Debug Drawing

The 2D API's shape catalog exists in 3D too, and needs no shader, mesh, or material at all:

```cpp
cf_draw3d_push_color(cf_color_red());
cf_draw3d_line(a, b, 0.05f);                            // Round caps, world-unit thickness.
cf_draw3d_arrow(from, to, 0.03f, 0.1f);                 // Line + solid cone head.
cf_draw3d_circle(center, normal, 1.0f, 0.02f);          // Plus circle_fill and arc.
cf_draw3d_box_wire(center, half_extents, 0.02f);        // 12 edges; the AABB debug classic.
cf_draw3d_axes(1.0f, 0.04f);                            // RGB local frame under the transform stack.
cf_draw3d_cube(center, half_extents);                   // Solids: cube, sphere, cone, torus,
cf_draw3d_pop_color();                                  // hemisphere-lit, colored by the stack.
```

Strokes render as camera-facing ribbons under a built-in signed-distance shader: edges are locally anti-aliased with no MSAA, and a stroke that falls below a pixel wide clamps to half-pixel width and fades its alpha instead of shimmering -- a distant wireframe grid stays calm. Thickness is in world units, so strokes recede with perspective like geometry, not screen overlays (`cf_draw3d_line2` varies thickness per-end for tapers).

Shape parameters ride the same reserved instance lanes as meshes, so strokes coalesce into instanced draws, order against layers, and record into draw lists -- a wireframe level bakes like anything else. Solids run through the ordinary mesh path with lazily-built unit meshes. See the `shapes3d` sample for the whole catalog moving.

## Sprite-Textured Meshes

Meshes can be textured straight out of CF's sprite economy:

```cpp
cf_draw3d_push_texture(&sprite); // Any sprite -- animated ones animate on the mesh.
cf_draw3d_mesh(quad);
cf_draw3d_pop_texture();
```

The image lives wherever the texture atlas compiler decides, the sub-rect arrives on the `in_uv_rect` instance lane, and the shader samples `texture(u_image, mix(in_uv_rect.xy, in_uv_rect.zw, in_uv))`. There is no atlas API to hold correctly -- and because drawing meshes together is itself the packing signal, many meshes with many different images still converge toward a single instanced draw. Mesh uvs must lie in [0, 1] (hardware wrap can't tile inside an atlas sub-rect); meshes with tiling uvs bind a standalone `CF_Texture` via `cf_draw3d_set_texture` instead. Mesh uv (0,0) samples the image's top-left, matching 2D sprites.

## Draw Lists and Baking

`CF_DrawList` records 3D submissions just like 2D drawing, and adds a bake: at `cf_draw_list_end`, submissions group by their full state *regardless of submission order*, each group's instances write out once, and replay issues one instanced draw per group. The city sample records 10,000 buildings plus ground into one list -- that's **one instanced draw per pass**, at four-digit framerates in a Debug build, with zero per-building CPU cost at replay.

Cameras are live at replay: a recorded level renders under whatever projection/view is current, and the current 3D transform stack moves the whole list for free. Baked instances also get exact inverse-transpose normal matrices (the immediate path reuses the model rows, exact for rigid transforms and uniform scale).

A recording behaves like a **closure**: state set *inside* it is part of the recording; state inherited from outside binds fresh each time the list draws. The transform stack always worked this way, and the shader and uniforms follow the same rule -- set inside `begin`/`end` they record frozen, but ambient state stays a *free variable*: `cf_draw_list` binds whatever is pushed or set then (record-time values as the fallback). That makes multi-pass rendering one-recording cheap:

```cpp
CF_DrawList city = cf_make_draw_list();
cf_draw_list_begin(city);      // No shader pushed: the shader stays free.
record_the_city();
cf_draw_list_end();

// Each frame -- same bake, one instanced draw per pass:
cf_draw3d_push_shader(shadow_shd);  cf_draw_list(city);  cf_draw3d_pop_shader();
cf_render_to(shadow_canvas, true);
cf_draw3d_push_shader(lit_shd);     cf_draw_list(city);  cf_draw3d_pop_shader();
```

Drawing a list under a pass shader also **fuses**: adjacent baked groups that differ only by frozen uniforms or textures the bound shader never declares collapse into a single draw -- a ten-material scene is one draw in a depth pass, because the depth shader reads none of the material state. `cf_draw3d_stats` counts draw-list draws, so the receipts show it.

And the same closure rule covers uniforms: a name set only *outside* the recording stays live -- set `u_time` each frame and a baked level animates; a name set *inside* the recording freezes with it. One rule to respect: ambient capture is by **name at record time** -- only names live when the recording's submissions were made participate, so set every per-frame uniform once *before* recording (bake at startup, before any uniform exists, and the replays bind no uniforms at all; the `fireflies` sample bakes on its first frame for exactly this reason).

Two things to know:

- Uniforms set inside a recording freeze their captures at record time. Per-frame camera-dependent values don't need uniforms anyway: the camera stacks deliver them, and view depth for fog arrives free as `gl_Position.w`. The one real trap: **uniform-driven skinning recorded into a list replays a single frozen pose** -- the bone bytes were snapshotted at the bake. Skinned characters inside lists use the storage-buffer pattern instead: the bake captures the buffer *handle* while its contents stay live, so updating the palette each frame animates replays normally (test-pinned in `test_draw3d_list_storage_buffer_live`).

## What Carries Over From the 2D API

One rule decides it: **state describing *where and when* a draw lands is shared; state describing *how pixels are produced* belongs to one domain or the other.**

Shared -- these cute_draw.h calls apply to mesh submissions too:

| Call | Effect on meshes |
| --- | --- |
| `cf_draw_push_layer` | Orders meshes against 2D drawing and each other |
| `cf_draw_push_scissor` / `cf_draw_push_viewport` | Captured per submission |
| `cf_make_draw_list` / `cf_draw_list_begin` / `end` / `cf_draw_list` | Records and bakes meshes |
| `cf_render_to` / `cf_render_layers_to` / `cf_app_draw_onto_screen` | The flush |

Everything else in cute_draw.h is 2D-geometry-only, and four concepts exist on *both* sides as parallel stacks with zero crossover: `cf_draw_push_shader` vs `cf_draw3d_push_shader`, `cf_draw_push_render_state` vs `cf_draw3d_push_render_state`, `cf_draw_set_uniform`/`_texture` vs the `draw3d` versions, and `cf_draw_push` (2D camera) vs `cf_draw3d_push` (3D transform).

## Conventions

- Right-handed. `cf_look_at` looks down **-z**; `cf_perspective`/`cf_ortho` take positive near/far distances along that axis.
- Clip-space z is **[0, 1]** (near maps to 0). Pair with `CF_COMPARE_FUNCTION_LESS_THAN` and a depth clear of 1. Reversed-Z works today if you want it -- flip the projection, the compare function, and the clear value; every piece is user-controlled.
- Front faces wind **counter-clockwise** under `cf_render_state_3d_defaults` (which enables depth write/test and back-face culling).
- `CF_M4x4` is column-major, uploadable as `CF_UNIFORM_TYPE_MAT4` with no transpose.
- Depth state only functions on a canvas created with `depth_stencil_enable` (the app's own canvas has it).

## The Samples

Each common 3D need has a sample showing the pattern, because each one is a pattern -- not missing framework:

| Sample | What it proves |
| --- | --- |
| `draw3d` | A 10,000-building city recorded ONCE, replayed per pass under each pass's shader (closure semantics: ambient shader + uniforms bind per pass); shadow-mapped sun via a comparison sampler (hardware PCF); fog; procedural window lights |
| `pixel_3d` | Multi-pass pixel-art pipeline: two shadow maps (color-encoded depth + hand-rolled PCF), lit pass, view-space g-buffer, 2D post-process composite |
| `skinning` | GPU skinning: joint/weight vertex attributes + a `mat4` array uniform; sixty strands, one shared skeleton, one instanced draw |
| `billboards` | Sprite-textured camera-facing quads: cutout trees (depth-ordered, no sorting) and additive fireflies (order-independent) |
| `transparency3d` | Sorted alpha done honestly -- opaque first, back-to-front translucents, with a toggle to watch unsorted blending break |
| `obj_loading` | A ~90 line OBJ parser into `cf_make_mesh`; model formats are user space, and this is the whole cost |
| `shapes3d` | The shape catalog moving: gizmos, grids, arcs, tapered lines, solids -- zero setup |
| `point_light` | Omnidirectional shadows: six face passes into one cube texture via `attach_target`, distance compare in the lit pass |
| `model3d` | glTF loading via `cf_make_model` (VFS-wired cute_model.h) and the storage-buffer skinning pattern: a five-fox pack on independent animation clips in three instanced draws |
| `fireflies` | The integration sample -- a first-person block forest with cascaded EVSM shadows, HDR bloom through render-to-mip canvases, and a full gameplay loop; the world bakes into per-chunk draw lists that are frustum-culled and replayed once per pass |

## 3D Math

[cute_math3d.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math3d.h) carries the vector/matrix/quaternion kit the camera stacks consume, plus the geometry that gameplay and culling want: `CF_Ray3` casts against spheres, AABBs, and planes (`cf_ray3_to_aabb3` for picking blocks, `cf_ray3_to_sphere` for picking things near a crosshair), `CF_Aabb3` transforms tightly through affine matrices (`cf_transform_aabb3`, the Arvo method), and `CF_Frustum` extracts from any view-projection with `cf_frustum_from_m4` for `cf_frustum_test_aabb3` culling -- the fireflies sample culls its chunks with exactly that pair.

## Below This Layer

The draw3d layer sits on the same [low level graphics API](low_level_graphics.md) everything else uses, and that layer grew the full 3D access inventory alongside it: multiple render targets with [per-target blend states](low_level_graphics.md#multiple-render-targets), cube/3D/array textures with per-layer and per-mip upload, depth-texture sampling and comparison samplers (`sampler2DShadow`), rendering into individual cube faces, array layers, or mip levels (`CF_CanvasParams.attach_target` / `attach_layer` / `attach_mip`), [storage buffers](low_level_graphics.md#storage-buffers-and-pull-instancing) for skinning palettes and GPU-driven data, [indirect draws](low_level_graphics.md#indirect-draws) fed by compute, standalone [samplers](low_level_graphics.md#standalone-samplers), and sized `vec4`/`mat4` arrays in uniform blocks. When the draw3d layer doesn't fit, drop down -- both layers speak the same meshes, shaders, materials, and canvases.

## Toward a Release

Headed for shipping? [Shipping 3D](shipping_3d.md) consolidates every per-backend caveat and pre-flight recipe -- compressed textures with mips, draw-list frustum culling, the web tier's capability list, the anti-aliasing options -- into one checklist.
