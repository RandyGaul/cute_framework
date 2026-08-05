# Model 3D

Loads a glTF fox with `cf_make_model` -- one call covering the VFS read, the
[cute_model.h](https://github.com/RandyGaul/cute_framework/blob/master/libraries/cute/cute_model.h)
parse, and external URI resolution -- decodes its embedded texture with
`cf_make_texture_from_model_image`, and runs a pack of five foxes through GPU skinning:
each fox on its own animation clip and phase, yet the whole pack lands in a handful of
instanced draws.

The trick is the storage-buffer skinning pattern:

- One `CF_StorageBuffer` holds every fox's bone palette back to back, updated once per
  frame with `cf_update_storage_buffer` and bound with `cf_draw3d_set_vs_storage_buffers`.
- Each fox's submission carries its palette's base offset in a free
  `cf_draw3d_push_mesh_attributes` lane, so changing animation state never splits the
  batch -- the vertex shader indexes the shared palette per instance.
- Buffer-block tails must be scalars or vectors, so the palette stores mat4s as four
  `vec4` columns and reassembles them with a tiny `bone()` helper in the shader.

The HUD prints `cf_draw3d_stats` live, which is the whole point: five animated characters,
sixty-plus instances, three draws. A skeleton overlay (built from `cf_draw3d_line` calls
over the posed joint hierarchy) shows the animation driving it all.

[:material-code-tags: View Source](https://github.com/RandyGaul/cute_framework/blob/master/samples/model3d.c){: .md-button target="_blank" }
