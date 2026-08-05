# Shipping 3D

Everything on this page is documented somewhere -- a header remark here, a sample comment there. This page is the consolidation: every per-backend caveat and pre-release recipe in one place, so each one costs you a checklist line instead of a debugging session. Read [3D Drawing](drawing_3d.md) first; this page assumes you're past "it renders" and headed for "it ships."

## The Backend Caveat Checklist

CF renders through SDL_GPU (Vulkan, Metal, D3D12, D3D11) plus its own GLES3 backend for web. The API is one surface, but the floors differ. `cf_query_backend()` tells you where you landed at runtime.

### Windows (D3D12 -- SDL_GPU's default driver there)

- **Depth cube faces don't render.** SDL_GPU's D3D12 driver creates only 2D depth views, so rendering into one face of a depth-format cube texture silently produces nothing. CF prints a loud warning when such a canvas is created. Use one of the portable patterns instead: six 2D depth canvases, a 2D depth atlas, or a color-encoded distance cube -- the `point_light` sample ships the last of these and it works on every backend.
- D3D11 is available behind `CF_APP_OPTIONS_GFX_D3D11_BIT` but is legacy-support only; don't target it fresh.

### macOS / iOS (Metal)

- No D24S8 depth format. `cf_canvas_defaults` negotiates to D32 float + S8 automatically -- only relevant if you construct depth `CF_TextureParams` by hand.

### Web (GLES3 / WebGL2)

The web tier trades capability for reach. If browsers are a release target, design these out from day one rather than discovering them at port time:

| Missing on web | Ship instead |
| --- | --- |
| Compute shaders, compute-writable storage buffers | CPU-side preparation, or gate the feature per backend |
| Indirect draws (hard assert) | Ordinary submissions; baked draw lists cover most GPU-driven wins |
| MSAA (none at all) | Post AA -- see the anti-aliasing stance below |
| BCn compressed textures | PNG/JPG fallbacks -- see the texture recipe below |
| Per-target blend states | `blends[0]` applies to all MRT targets |
| Depth clamp (`enable_depth_clip` ignored) | Pull the shadow near plane back instead |
| GPU debug labels | No-ops; harmless to leave in |
| Async readback | Same API, but it stalls the pipeline -- keep it out of the frame loop |

Read-only storage buffers *do* work on web -- emulated through texture fetches, transparently -- limited to 4 per stage, each an anonymous block with a single runtime `vec4[]` tail. The storage-buffer skinning pattern ships on web unchanged (`model3d --gles` exercises exactly this).

### Everywhere

- **MSAA and MRT are mutually exclusive** -- a multi-target canvas requires `CF_SAMPLE_COUNT_1`.
- **MSAA targets can't be sampled directly.** The resolve happens automatically at pass end; `cf_canvas_get_target` hands you the resolved texture, which is the one you sample.
- **No wireframe fill mode.** Rasterizer fill is always solid. Debug wireframes are what the 3D stroke shapes are for (`cf_draw3d_box_wire` and friends -- anti-aliased, batched, no state to manage), or build a `CF_PRIMITIVE_TYPE_LINELIST` mesh.
- **Front faces wind counter-clockwise, not configurable.** Imported meshes wound clockwise need their index order flipped at import (or `CF_CULL_MODE_FRONT` as a blunt instrument).
- **No base-vertex in range draws.** `cf_draw3d_mesh_range` / `cf_draw_elements_range` take absolute indices -- geometry arenas write their indices absolute into the shared buffer. This is what keeps ranges portable to web.
- **Sampling a depth texture needs one of two things:** `compare_enable` on its `CF_TextureParams` (hardware-compare shadow sampling via `sampler2DShadow`), or a standalone `CF_Sampler` bound alongside it (raw depth reads, e.g. PCSS blocker search). A depth texture with neither gets no sampler at all.
- **A canvas depth target isn't sampleable by default.** `cf_canvas_defaults` gives the depth attachment `DEPTH_STENCIL_TARGET_BIT` only -- OR in `CF_TEXTURE_USAGE_SAMPLER_BIT` yourself before sampling it in a later pass (the `draw3d` sample's shadow canvas shows the full setup).
- **No GPU timing.** SDL_GPU exposes no timestamp or occlusion queries yet, so neither does CF. Profile with `cf_push_gpu_label`/`cf_pop_gpu_label` regions under RenderDoc, Nsight, or PIX; watch batching with `cf_draw3d_stats`; and use `cf_gpu_sync` for coarse CPU-side bracketing when desperate.

## The Anti-Aliasing Stance

CF takes no AA position for you, but the pieces line up like this:

- **3D strokes self-anti-alias.** The signed-distance ribbon shader gives smooth edges at any zoom with no MSAA involved -- debug and stylized line work is covered for free.
- **MSAA covers desktop forward rendering** -- single target, `sample_count` on the canvas, automatic resolve. It stops at the MRT boundary and doesn't exist on web.
- **Everything else is post AA in user shaders.** FXAA is an afternoon; TAA is a weekend plus motion vectors (`cf_draw3d_set_uniform_m4` with last frame's view-projection is the input it needs). Deferred pipelines and web builds end up here, which is the same place most shipped engines ended up.

## Recipe: Shipping Textures

`cf_make_texture_from_model_image` decodes a model's embedded PNG/JPEG at load and generates a full mip chain -- the right development default, and fine to ship for small games. Past that, runtime decode burns load time and uncompressed pixels burn memory and bandwidth.

**Native targets: convert offline to DDS.** glTF embeds PNG/JPEG; a shipping build converts those to block-compressed DDS in the asset pipeline (BC7 for color, BC5 for normal maps, BC4 for single-channel masks -- any standard tool: NVTT, Compressonator, ktx/toktx piped through DDS). Then:

```cpp
CF_Texture tex = cf_make_texture_from_dds("/assets/fox_albedo.dds");
```

One call uploads the compressed pixels and the full mip chain exactly as authored -- no decode, no runtime mip generation. Use the sRGB BC variants for color textures and linear for data textures, and `cf_texture_supports_format` if you want to probe before committing.

**Web fallback: keep the PNGs, generate mips.** There's no BC on WebGL2, so ship the originals there -- but never mipless:

```cpp
CF_TextureParams tp = cf_texture_defaults(w, h);
tp.allocate_mipmaps = true;   // mip_count 0 = full chain
tp.filter = CF_FILTER_LINEAR;
tp.usage |= CF_TEXTURE_USAGE_COLOR_TARGET_BIT; // cf_generate_mipmaps downsamples via GPU blits.
CF_Texture tex = cf_make_texture(tp);
cf_texture_update(tex, pixels, size);
cf_generate_mipmaps(tex);
```

Anisotropic filtering (`max_anisotropy` on `CF_TextureParams`) is the difference between smeared and readable ground textures at grazing angles; it's cheap on everything modern.

## Recipe: Culling Baked Draw Lists

A baked `CF_DrawList` replays as one instanced draw -- which also makes it all-or-nothing: there is no per-instance culling inside a bake. The pattern is to make the *list* the culling granularity:

```cpp
// At load: record one list per spatial chunk, each with its bounds.
struct Chunk { CF_DrawList list; CF_Aabb3 bounds; };

// Per frame: extract the frustum once, replay only what it sees.
CF_M4x4 view_projection = cf_mul(projection, view);
CF_Frustum frustum = cf_frustum_from_m4(view_projection);
for (int i = 0; i < chunk_count; ++i) {
	if (cf_frustum_test_aabb3(frustum, chunks[i].bounds)) {
		cf_draw_list(chunks[i].list);
	}
}
```

Chunks of a few hundred to a few thousand instances keep the draw count trivial while letting the frustum do real work -- the `fireflies` sample culls its forest chunks with exactly this pair. Size chunks so a typical view rejects most of them; a single worldwide list rejects nothing.

When frustum granularity isn't enough -- dense cities, per-instance occlusion -- the escape hatch composes: meshes with their own instance buffers, a compute pass that culls into a storage buffer, and `cf_draw_elements_indirect` consuming the result, all with zero readback. SDL_GPU backends only; on web, chunked lists are the answer.

## Pre-Flight, Compressed

- [ ] Windows/D3D12 run tested -- especially anything shadow-cube shaped
- [ ] Web build tested early if browsers are a target (compute/indirect/MSAA/BC gaps designed out, not ported out)
- [ ] Textures block-compressed with mips on native; mipped PNG fallback on web; anisotropy on
- [ ] MSAA only on single-target canvases, or post AA
- [ ] Draw lists chunked for frustum culling
- [ ] `cf_draw3d_stats` near zero avoidable splits; `cf_push_gpu_label` regions in place for GPU captures
