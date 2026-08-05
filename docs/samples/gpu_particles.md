# GPU Particles

65,536 embers whose entire life cycle lives on the GPU -- the sample that proves CF's
GPU-driven loop end to end:

- A compute shader simulates every particle in place inside one `CF_StorageBuffer`
  (`compute_writable` + `graphics_readable`): swirl, buoyancy, and hashed respawn on an
  emitter ring, no CPU-side particle data at all.
- A second one-thread compute pass writes the draw's `CF_DrawIndirectArgs` block --
  four uint32s, exactly one `uvec4` -- ramping the live instance count on the GPU.
- The render is pull instancing: the mesh holds six corner vertices and nothing else;
  the vertex shader reads position, life, and velocity from the same storage buffer by
  `gl_InstanceIndex`, billboards against the camera, and the draw is issued with
  `cf_draw_elements_indirect`. The CPU never learns how many particles are alive.

Simulate, write args, draw -- zero readback. This is the modern counterpart to the
`galaxy` sample, which predates storage buffers and round-trips its state through
textures. SDL_GPU backends only: compute and indirect draws don't exist on GLES3/web.

Harness: `--shot <t>` saves a numbered screenshot, `--exit-at <t>` quits cleanly.

[:material-code-tags: View Source](https://github.com/RandyGaul/cute_framework/blob/master/samples/gpu_particles.c){: .md-button target="_blank" }
