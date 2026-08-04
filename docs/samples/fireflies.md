# Fireflies

A first-person forest at dusk, built entirely from blocks. Wander a Zelda-ish woodland vale
until you find a glass jar on a stump, catch the fireflies drifting through the glades (the
jar doubles as your lantern -- brighter with every catch), then carry your light to the
dormant shrine in the great clearing and set them free. When all the lanterns wake, dawn
comes early.

Controls: WASD to move, mouse to look, E to interact, left click to catch a firefly near
the crosshair.

This is the 3d API's integration sample -- the pieces working as one system rather than in
isolation:

- Thousands of blocks flow through `cf_draw3d_mesh` every frame and coalesce automatically
  into a handful of instanced draws. Block color and emissive strength ride
  `in_mesh_attributes`; leaf sway rides the free second lane (`in_uv_rect`).
- Two shadow cascades render into the layers of one depth 2d-array texture
  (`CF_CanvasParams::attach_target` + `attach_layer`) and are sampled with hardware PCF
  through `sampler2DArrayShadow`.
- HDR bloom runs through render-to-mip canvases (`CF_CanvasParams::attach_mip`): a bright
  pass, then a downsample chain that ping-pongs between the mip levels of two textures so
  no pass ever samples the texture it writes.
- Frustum culling, jar pickup, and firefly catching all go through `cute_math3d.h`'s
  geometry: `cf_frustum_from_m4` + `cf_frustum_test_aabb3` over chunk bounds, and
  `cf_ray3_to_sphere` from the camera ray.
- The world renders in HDR (`CF_PIXEL_FORMAT_R16G16B16A16_FLOAT`), and one fullscreen pass
  composites bloom, tonemaps (ACES), and vignettes into the app canvas.

The sample can also play itself: every control flows through one `Input` struct, so
`fireflies --auto` runs a scripted tour of the entire loop -- find the jar, hunt fireflies,
wake the shrine -- with no human input. `--shot <seconds>` (repeatable) saves numbered
screenshots and `--exit-at <seconds>` quits cleanly, which together make the sample its own
smoke test and its own screenshot rig.
