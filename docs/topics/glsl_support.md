# GLSL Support (CF-GLSL)

CF compiles shaders with its own built-in compiler, `cute_spirv`. It compiles a subset of GLSL 450 directly to SPIR-V with zero external dependencies — no glslang, no Python, nothing to fetch or install. The subset is defined by what CF's shaders and samples actually need, and it's documented precisely here so there are no surprises.

Two things to know up front:

1. **The subset is deliberate, not a limitation of ambition.** Every terminal consumer of a shader (Vulkan drivers, Metal's compiler, D3D compilers, GL drivers) runs its own full optimizing compiler, so CF's compiler stays small and fast by design.
2. **CF fully owns this pipeline.** There is no upstream committee or external dependency deciding what's supported. If you need something that isn't listed below, [open an issue](https://github.com/RandyGaul/cute_framework/issues) — expanding the subset is usually straightforward, and requests are welcome.

## Supported

### Stages

Vertex, fragment, and compute.

### Types

- Scalars: `bool`, `int`, `uint`, `float`
- Vectors: `vec2/3/4`, `ivec2/3/4`, `uvec2/3/4`, `bvec2/3/4`
- Square matrices: `mat2`, `mat3`, `mat4`
- Structs (declaration, constructors, field access, nesting in arrays, use as function parameters/returns)
- Sized arrays of any of the above, including `T[](...)` and `T[N](...)` constructors and `.length()`; both `vec2[8] v` and `vec2 v[8]` declarator forms
- Opaque types: `sampler2D`, `usampler2D`, formatted `image2D` (formats: `rgba32f`, `rgba16f`, `rg32f`, `rg16f`, `r32f`, `r16f`, `rgba8`, `rgba8ui`, `r32ui`)
- Precision qualifiers (`highp`/`mediump`/`lowp`) are accepted and ignored

### Declarations

- `layout(location = N) in/out` variables, with `flat` interpolation (integer fragment inputs are implicitly flat)
- `layout(set = N, binding = M) uniform` blocks (std140) — scalars, vectors, and `mat4` members — anonymous or with instance names (`uniform Params { ... } u_params;`)
- `layout(std430, set = N, binding = M) [readonly] buffer` storage blocks, anonymous or instance-named, with struct members and runtime array tails of scalars/vectors/structs (`Cmd cmds[];`), whole-struct loads (`Cmd c = cmds[i];`), and `.length()`
- Global variables, `const` globals with constant-expression initializers, multi-declarator declarations (`int i = 0, j = N - 1;`)
- Array sizes may be integer constant expressions (`shared vec3 s_tile[TILE_W * TILE_W];`, `float k[N + 2];` with `const int N`)
- Compute: `layout(local_size_x = ..., ...) in;`, `shared` variables (including arrays)
- Function definitions with overloading; `in`, `out`, and `inout` parameter qualifiers (copy-in/copy-out semantics)

### Statements and expressions

- `if`/`else`, `for`, `while`, `do-while`, `switch` (integer literal cases, fallthrough, `default`), `break`, `continue`, `return`, `discard`
- Full expression grammar: arithmetic/bitwise/shift/comparison/logical operators, ternaries, swizzles (including swizzle assignment like `v.xy = ...`), compound assignment, `++`/`--`, vector/matrix/array subscripts with dynamic indices, the comma operator in `for` clauses
- Implicit conversions `int -> uint -> float`, scalar-to-vector splatting in operators, constructors, and intrinsics

### Preprocessor

`#include` (with automatic include guards — no `#pragma once` needed), object- and function-like `#define`, `#undef`, `#if`/`#ifdef`/`#ifndef`/`#elif`/`#else`/`#endif` with `defined()`, line continuations. `#version`, `#extension`, and `#pragma` are accepted and ignored. Errors report the correct file and line across includes.

### Builtins

- `gl_Position`, `gl_VertexIndex`, `gl_InstanceIndex` (vertex); `gl_FragCoord` (fragment); `gl_GlobalInvocationID`, `gl_LocalInvocationID`, `gl_LocalInvocationIndex`, `gl_WorkGroupID`, `gl_NumWorkGroups` (compute)
- ~90 intrinsics: the standard math set (`mix`, `clamp`, `pow`, `smoothstep`, trig, exp/log, ...) with int/uint variants where GLSL defines them, geometric functions (`dot`, `cross`, `normalize`, `length`, `distance`, `reflect`, `refract`), matrix functions (`transpose`, `inverse`, `determinant`), vector relationals (`lessThan`, `equal`, `all`, `any`, `not`, ...), derivatives (`dFdx`, `dFdy`, `fwidth`), bit casts (`floatBitsToUint` & co.), pack/unpack (`packUnorm4x8`, `packHalf2x16`, ...), texturing (`texture`, `textureOffset`, `textureLod`, `texelFetch`, `textureSize`), storage images (`imageLoad`, `imageStore`, `imageSize`), atomics (`atomicAdd`/`Min`/`Max`/`And`/`Or`/`Xor`/`Exchange`/`CompSwap` on buffer or shared memory), and barriers (`barrier()`, `memoryBarrier*`, `groupMemoryBarrier`)

## Not supported

These are outside the subset today. None of them appear in CF's shaders or samples — if your use case needs one, say so in an issue and it can likely be added:

- `double` precision, non-square matrices (`mat4x3` etc.)
- Geometry/tessellation stages, subroutines
- `sampler3D`, `samplerCube`, sampler arrays, shadow samplers
- Multi-dimensional arrays; sized arrays inside uniform/buffer blocks; nested structs inside buffer-block structs
- Stringize (`#`) and paste (`##`) preprocessor operators
- `precise`, `invariant`, specialization constants

## Known semantic notes

- `&&`, `||`, and ternaries short-circuit correctly: only the taken side evaluates.
- A non-void function whose final block falls off the end after at least one `return` (e.g. an exhaustive `if/else`) compiles fine; a function with no `return` at all is an error.
- Shader compilation is not thread-safe (it shares a global string-intern table); compile shaders from one thread at a time. The old glslang pipeline had process-global state too, so this is not a regression.

## Upgrading from the glslang pipeline

If you're coming from a CF version that used glslang, three things to know:

1. **Regenerate precompiled shaders.** Headers produced by the old `cute-shaderc` must be regenerated with the new one. `CF_ShaderBytecode` has always been documented as opaque with no ABI guarantee, so this is the expected workflow when updating CF — same commands, same flags, smaller output.
2. **Shaders outside the subset now error with a named message.** glslang accepted all of GLSL 450; CF's compiler accepts the subset above. If one of your shaders uses something outside it, you'll get a precise error naming the construct — [open an issue](https://github.com/RandyGaul/cute_framework/issues) and it can usually be added quickly. Everything CF itself and all of its samples use is covered.
3. **Nothing to install anymore.** Python is no longer needed to build CF and no sources are fetched for the compiler. On Windows nothing ships next to executables either: D3D12 shaders are transpiled to HLSL by cute_spirv and compiled to DXBC by the system's own FXC (`d3dcompiler_47.dll`).

One behavioral note: bytecode produced by *runtime* compilation only targets the backend the app is running on (the GLSL ES 300 payload is skipped elsewhere). Bytecode from the offline `cute-shaderc` always contains everything and works on every backend.

## Why a custom compiler?

The previous pipeline pulled in [glslang](https://github.com/KhronosGroup/glslang) (a ~136 MB source fetch requiring a Python installation) to do the exact same job. Since drivers re-optimize all shader code anyway, CF's compiler skips optimization entirely and simply emits straightforward SPIR-V — making it a small, dependency-free part of CF itself, with better error messages and full control over the language. See [Shader Compilation](shader_compilation.md) for how compilation fits into the build, and how precompiling shaders works.
