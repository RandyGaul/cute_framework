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
- Two shadow cascades render EVSM moments (`exp(c*z)` and its square) into the layers of
  one RG32F 2d-array texture (`CF_CanvasParams::attach_target` + `attach_layer`), get a
  separable gaussian blur, and resolve with a Chebyshev bound -- soft, acne-free shadows
  with no depth bias to tune, and light bleeding clamped by one `linstep` remap.
- HDR bloom runs through render-to-mip canvases (`CF_CanvasParams::attach_mip`): a bright
  pass, then a downsample chain that ping-pongs between the mip levels of two textures so
  no pass ever samples the texture it writes.
- Frustum culling, jar pickup, and firefly catching all go through `cute_math3d.h`'s
  geometry: `cf_frustum_from_m4` + `cf_frustum_test_aabb3` over chunk bounds, and
  `cf_ray3_to_sphere` from the camera ray.
- The world renders in HDR (`CF_PIXEL_FORMAT_R16G16B16A16_FLOAT`), and one fullscreen pass
  composites bloom, tonemaps (ACES), and vignettes into the app canvas.

The jar is a proper little lantern: a bronze frame around a translucent glass body (glass
opacity rides a free instance lane and composites through draw3d's automatic translucent
sort), with a warm core that burns brighter for every carried firefly -- and the catch
itself drifting around inside the glass. The same assembly draws the stump pickup, the
held lantern, and the shrine's six lanterns.

The sample can also play itself: every control flows through one `Input` struct, so
`fireflies --auto` runs a scripted tour of the entire loop -- find the jar, hunt fireflies,
wake the shrine -- with no human input. `--shot <seconds>` (repeatable) saves numbered
screenshots, `--record <seconds>` dumps fixed-timestep frames for ffmpeg, and
`--exit-at <seconds>` quits cleanly, which together make the sample its own smoke test,
screenshot rig, and trailer camera.
