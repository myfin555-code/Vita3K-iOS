# Vita3K-Plus graphics comparison and iOS port

## Compared revisions

- Official: [`610e6970ad943d281139bc9081fa82cfb2498068`](https://github.com/Vita3K/Vita3K/tree/610e6970ad943d281139bc9081fa82cfb2498068).
- Plus **all-enhancements**: [`20588fbf52370356b8e5e8ca52ed3dd59ba8ddad`](https://github.com/nckstwrt/Vita3K-Plus/tree/20588fbf52370356b8e5e8ca52ed3dd59ba8ddad).
- Shared base: `5b040dc4a6373ccecfaeeaaf817ba0edb8241edd`.
- iOS before this port: `3ac977a4fbce7933e2ad3e194fe1ab537ecdc9f4`.

The Plus README directs code comparisons to **all-enhancements**. Comparing only
Plus master (`c44cec34`) misses the fixes. Against the common base, the actual
fix branch changes **216 files**, with **13,526 insertions and 1,633 deletions**
and 116 non-merge commits. Current official changes 104 files from that base;
32 paths changed on both sides. A direct official→Plus diff also contains
upstream evolution absent from Plus, so it must not be treated as a patch to
apply wholesale to iOS.

| Area | Changed files |
| --- | ---: |
| Graphics, shaders, GXM and Vulkan utilities | 67 |
| CPU, kernel and memory | 27 |
| Audio, video and camera | 24 |
| Input and IME | 7 |
| I/O, services and saved data | 16 |
| Android frontend/platform | 32 |
| UI, configuration, integration and diagnostics | 43 |

These are path-based inventory categories, not counts of independent bug fixes.

## Why Plus can render a game differently

The important changes are corrections to emulated GPU behavior:

- USSE complex instructions use base-2 exponent/logarithm. Translating them as
  natural exponent/log changes lighting and color calculations.
- Packing, register repetition, predicate handling, sampler lookup and uniform
  register mapping determine the values reaching the pixel shader.
- Games reinterpret the same image memory with different formats. Plus adds
  compute reinterpretation, raw F16 bit preservation, swizzle/format handling
  and high-resolution sample-coordinate corrections. Ordinary filtering or
  blitting can change the bits that the next pass expects.
- Depth/stencil lifetime and surface ownership decide whether later passes see
  current depth and pixels. Plus also changes scene/writeback ordering and
  program lifetime snapshots.
- Scheduling, audio/video and service implementations prevent some stalls that
  can look like a rendering freeze. They are separate from GPU image accuracy.

Example upstream explanations are in Plus commits
[`01f6dc6e`](https://github.com/nckstwrt/Vita3K-Plus/commit/01f6dc6e)
(base-2 math/Killzone),
[`f4e40524`](https://github.com/nckstwrt/Vita3K-Plus/commit/f4e40524)
(cube textures/DOA5),
[`7531b756`](https://github.com/nckstwrt/Vita3K-Plus/commit/7531b756)
(high-resolution typeless copies), and
[`5e0e89d2`](https://github.com/nckstwrt/Vita3K-Plus/commit/5e0e89d2)
(writeback/freeze handling). Those game descriptions are the author's reports;
they are not iPhone compatibility results from this port.

## Changes ported here

Selected code is adapted from the GPL-licensed Plus branch above; existing
Vita3K copyright/license notices are retained.

| Area | Integrated behavior |
| --- | --- |
| Complex ALU | VLOG/FLOG → Log2; VEXP/FEXP → Exp2; preserve the existing NaN workaround |
| ALU repetition | Honor VNMAD SMLSI/SMBO offsets, integer repeat slots/counts and internal-register increments |
| Scalar results | Broadcast scalar dual-instruction results to every enabled destination component |
| Predicates | Implement vector NEGP2 through decode, control flow and disassembly |
| Branch analysis | Replace the moved code node rather than shadowing it; start the new node at the target; stop at out-of-block jumps |
| GXP uniforms | Read secondary-register block size/base from the matching uniform container |
| Conditional moves | Broadcast scalar conditions for vector OpSelect in SPIR-V 1.0 |
| Texture-buffer sampler lookup | Resolve dependent-sampler layout instead of assuming buffer slot equals texture unit |
| GPU pointers | Sign-extend offsets during 64-bit word-pair addition, bound decoded buffer indices and reject implausibly large decoded offsets; align physical accesses and keep signed/unsigned SPIR-V types consistent |
| Packing primitives | Saturate unnormalized 8/16-bit conversions; sanitize NaNs; use constant component extraction/insertion when indices are static |
| Register initialization | Initialize private register banks before partial writes |
| Fragment inputs | Support the GLOBAL g16 front-facing flag; reuse one builtin for OpenGL initialization |
| sRGB | Exact piecewise encode/decode for the existing storage-image path; declare required extended storage-image format capability |
| Texture LOD | Correct split min-LOD bits in the guest setter/getter and both rendering backends |
| Cube textures | Bypass 2D surface-cache views when a cube sampler needs six faces |
| Blend state | A disabled color or alpha blend channel uses ONE/ZERO when the other channel enables blending |
| MSAA target binding | Restore original target dimensions before expansion on each binding |
| Staging reuse | Avoid unsigned subtraction underflow during startup; initialize used byte count |
| Presentation | Reuse the finished semaphore by acquired swapchain image, following the [Khronos rule](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html) |
| Cache migration | Shader cache version 16 prevents loading pre-port shader binaries |

### Adaptations required during integration

- Keep scalar predicate encoding `PN = 7`. Plus inserts NEGP2 before PN;
  this port gives NEGP2 a synthetic value after PN and explicitly maps vector
  encodings, preserving existing scalar/branch instruction meanings.
- Omit Plus's 32-bit float saturation endpoints: `4294967295.0f` and
  `2147483647.0f` round out of range in float32. The imported saturation applies
  to representable 8/16-bit endpoints only.
- Keep the existing uniform ABI. No inactive high-resolution cast fields or
  unsupported Vulkan features are exposed as working iOS settings.
- Preserve iOS DoubleBuffer/disabled mapping, staging readback, texture source
  lifetime locking, disabled mprotect tracking, JIT pool and memory budgets.
- Fix two build issues found by compiling the actual renderer: include the full
  Config definition in Vulkan texture.cpp, and bridge the exception rename in
  Vulkan-Hpp 1.4.323+ for the pinned VMA-Hpp. MoltenVK 1.4.2 includes header 357.
  The dependency revision itself is unchanged.

The offset guard is crash mitigation, not proof that every guest-generated GPU
address belongs to a valid allocation. Device validation is still required.

## Plus changes deliberately not enabled by this patch

| Group | Reason / remaining integration work |
| --- | --- |
| Typeless compute casts, raw F16 attachment, 4444/RGB9E5 emulation, high-resolution UV masks | Coupled surface-cache, shader-uniform, descriptor and pipeline changes. Need format-capability checks and image comparisons on Apple GPUs, including native and scaled resolutions. The isolated math fixes do not reproduce this entire path. |
| Depth/stencil validity, clip planes, iterator masks and pipeline-key redesign | Require coherent scene ownership, output-format hints and pipeline/state changes; need depth/blending scene regression captures. |
| Program-binding snapshots and release/deferred destruction | Crosses guest handles, command payload ownership, asynchronous shader workers and teardown. Requires lifetime/concurrency integration; no claim that all rapid-input/game-exit crashes are fixed here. |
| Full texture mip/cube footprint tracking | Plus's early one-mip return excludes other cube faces; verify layout/face padding against the uploader before adopting it. iOS's existing source lifetime validation remains. |
| PageTable/NativeBuffer/ExternalHost and Turnip/Mali/Adreno rules | Depend on platform-specific memory import/protection/driver behavior not provided by the iOS backend. |
| Guest-core scheduler/deadlock breaker | Plus's guest scheduling and iOS's JIT execution pool model different resources. Artificial timeouts/wakeups must not be inserted without synchronization tests. |
| NGS, H264, camera, I/O delay, save/load, Location/PSN stubs, extra IME event queue | Separate guest-service behavior; catalogued in the full comparison, not included in this graphics patch. |
| Android/Qt UI, driver installer, branding/updater defaults | Not iOS frontend implementations. |

## Validation and limits

The host suite in [tests/shader](tests/shader/README.md) compiles real shader/GXM
sources, checks generated SPIR-V and translates synthetic GXP fixtures to iOS
Metal source. The ordinary native suite includes staging frame lifetime tests.
The changed Vulkan creation/context/texture/presentation translation units were
syntax-checked with `VITA3K_PLATFORM_IOS=1`, the actual MoltenVK 1.4.2 headers and
pinned dependencies. The production GXM LOD API bodies were also compiled and
exercised independently. These checks found and resolved the build issues above.

No Apple SDK or iPhone is available in the Linux sandbox. Full IPA compile/link,
Metal compilation, synchronization validation, screenshots and FPS/crash testing
in Attack on Titan (and the games named by Plus) remain unverified. Start device
comparison at 1x resolution and identical game/settings; compare shader-cache
rebuild, lighting, alpha, cube reflections, repeated scene binds and rapid video
skip input before increasing resolution.
