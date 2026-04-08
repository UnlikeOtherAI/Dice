# Dice — 3D RPG Dice Rendering Library

## What

A cross-platform C++ library that renders interactive 3D polyhedral dice (D&D-style) embeddable in iOS and Android apps via native Swift and Kotlin wrappers.

## Why

No lightweight, embeddable, production-quality 3D dice library exists for mobile. Existing solutions are either web-only (Three.js dice boxes), tied to full game engines (Unity/Unreal), or visually poor. This library fills the gap: a small, focused C++ core with native mobile wrappers that looks good out of the box.

## Requirements

### Dice Types

Variable-sided polyhedra — the library must support at minimum:

| Die | Faces | Shape | Geometry Notes |
|-----|-------|-------|----------------|
| d4 | 4 | Tetrahedron | Standard Platonic solid |
| d6 | 6 | Cube | Standard Platonic solid |
| d8 | 8 | Octahedron | Standard Platonic solid |
| d10 | 10 | Pentagonal trapezohedron | 12 vertices, 10 kite-shaped quad faces. Two distinct edge lengths (short ~0.618, long ~1.618). See [d10 geometry](#d10-pentagonal-trapezohedron) for exact coordinates. |
| d12 | 12 | Dodecahedron | Pentagonal faces — triangulate as fan from face centre for UV mapping |
| d16 | 16 | Octagonal dipyramid | 2n triangular faces from n=8 equator ring + two apices (see [research](research/filament-cross-platform-dice.md#d16-as-an-octagonal-dipyramid)) |
| d20 | 20 | Icosahedron | Standard Platonic solid |
| d32 | 32* | Pentakis dodecahedron | **60 triangular faces, 32 vertices** — "d32" is a misnomer; need number->face mapping with repeats, or treat as d60 (see [research](research/filament-cross-platform-dice.md#d32-as-a-pentakis-dodecahedron-face-count-mismatch-and-what-it-implies)) |

**d32 decision:** Use option (a) — map values 1-32 across the 60 faces (28 values appear twice, 4 values appear once, totaling 60). Since outcomes are predetermined by the caller, all that matters is that each value maps to at least one face orientation. The caller picks a result; the library picks a face instance and orients it toward the camera.

The die type is configurable — pass the face count and the library selects the correct geometry. Arbitrary face counts beyond the standard set are a stretch goal.

### Visual Appearance

**Rounded edges:** All polyhedra use parametric edge chamfering — each sharp edge is replaced with a narrow bevel face, and each vertex corner with a small polygon cap. This matches the look of real physical dice.

- `bevel_factor` parameter: `0.0` = mathematically sharp, `0.05` = default (mild, like real dice), `0.0–0.15` is the useful range
- Implemented via face-inset: each face is shrunk toward its centroid by `bevel_factor`, the gaps between adjacent inset faces are filled with new quad bevel faces, and vertex corners are capped with N-gon faces
- Applies to all die types including d4, d6, d8, d10, d12, d16, d20, d32

**Colors (configurable per die instance):**
- `dieColor` — RGBA color of the die body (e.g., deep red, midnight blue, bone white)
- `numberColor` — black or white (binary; no arbitrary RGB for numbers to maintain legibility)

### Interaction Model

The die **floats in space** — there is no table, no surface, no gravity simulation. It levitates in the center of the view.

On command, the die spins freely in 3D space (tumbling rotation on multiple axes), then decelerates and settles with the target face toward the camera.

**Multiple dice:** The view supports N simultaneous dice. D&D requires this — rolling 2d6 for damage, 2d20 for advantage/disadvantage, 4d6 for character stat generation, etc. Each die is an independent entity with its own geometry, color, and animation state.

The host app interacts through a public API:

```
// Die management
addDie(sides: Int, dieColor: Color, numberColor: NumberColor) -> DieHandle
removeDie(handle: DieHandle)

// Rolling
roll(handle: DieHandle, result: Int, spinDuration: Float)
rollAll(rolls: [(DieHandle, result: Int)], spinDuration: Float)

// Frame update (called by display link / Choreographer)
tick(deltaTime: Float)
```

- **result** — the number the die must land on (predetermined outcome)
- **spinDuration** — how long the die spins before settling (seconds)
- **DieHandle** — opaque identifier for a die instance

The library uses an **update-driven** model — the host app (or an internal display link) calls `tick(dt)` each frame, and the animation controller advances state.

**No physics engine required.** This is pure rotation animation using the tumble + correction quaternion approach:

1. Generate a tumble trajectory by integrating randomized multi-axis angular velocity (`axis = normalize(omega)`, `angle = |omega| * dt`, multiply current quaternion by delta, renormalize)
2. Compute where the tumble would naturally end: `q_raw_end`
3. Compute correction: `q_corr = inverse(q_raw_end) * q_target`
4. Blend correction in over time — near 0 for the first ~70% of duration, then rises smoothly to 1 via ease-in curve
5. At `t = T`, correction is fully applied — die stops exactly on the target face

Angular velocity decays using easing functions. Best candidates for floating dice feel:
- **easeOutCubic** — smooth deceleration, natural feel
- **easeOutQuint** — stronger "fast start, long tail", feels airy/inertial (recommended)
- **easeOutExpo** — punchy start, good for short spins

**Shortest-path slerp gotcha:** if `dot(qA, qB) < 0`, flip one quaternion to avoid spinning the long way around.

**Correction timing gotcha:** starting correction before ~70% looks like "steering"; starting after ~85% causes a visible snap.

### Rendering

- 3D die rendered in its own view/surface, compositable into native UI
- The die floats in empty space — no table, no environment, just the die
- Visually polished: PBR materials, lighting, shadows (on the die itself)
- Customizable: die color, number color/font, background (transparent or solid)
- Performant: 60 fps on mid-range devices, minimal battery impact when idle

**Transparency:** Background is always transparent — host app composites the dice over its own UI. Uses `CONFIG_TRANSPARENT` SwapChain flag + `View::BlendMode::TRANSLUCENT`. Known gotcha: artifacts on some Android devices — test on hardware, support both `SurfaceView` and `TextureView`.

### Architecture

```
┌─────────────────────────────────────┐
│         Host App (Swift/Kotlin)     │
│  ┌───────────┐   ┌───────────────┐  │
│  │ DiceView  │   │ DiceController│  │
│  │ (UIView/  │   │ .roll(face,   │  │
│  │  Surface) │   │   duration)   │  │
│  └─────┬─────┘   └───────┬───────┘  │
│        │                 │          │
├────────┼─────────────────┼──────────┤
│        ▼                 ▼          │
│  ┌──────────────────────────────┐   │
│  │      C++ Core (libdice)      │   │
│  │  ┌──────────┐ ┌───────────┐  │   │
│  │  │ Renderer │ │ Animation │  │   │
│  │  │(Filament)│ │ Controller│  │   │
│  │  └──────────┘ └───────────┘  │   │
│  │  ┌──────────┐ ┌───────────┐  │   │
│  │  │ Geometry │ │   Face    │  │   │
│  │  │Generator │ │  Mapper   │  │   │
│  │  └──────────┘ └───────────┘  │   │
│  └──────────────────────────────┘   │
│        │                 │          │
│  Metal (iOS)    Vulkan/GLES (Android)│
└─────────────────────────────────────┘
```

**C++ core (`libdice3d_core`)** — all rendering and animation logic. Single codebase. Exposes a **C ABI surface** (`extern "C"`) for JNI simplicity and to avoid Swift C++ interop constraints. The library **owns** the Filament `Engine` and scene graph; the host app only provides a native surface and input.

**Key components:**

- **Renderer** — Filament-based scene: `Engine`, `SwapChain`, `Renderer`, `View`, `Scene`, `Camera`. Single render thread affinity (Filament `Engine` is not thread-safe). Must call `endFrame()` if `beginFrame()` returns true, even if skipping draw.
- **Animation Controller** — manages spin state: idle -> spinning -> decelerating -> settled. Quaternion integration from angular velocity each frame. Tumble + correction blend for predetermined outcome.
- **Geometry Generator** — procedural mesh generation for each polyhedron type. Vertices duplicated per face for independent UVs. Tangent frames generated using Filament's `SurfaceOrientation` utility (Filament stores tangents as quaternions).
- **Face Mapper** — precomputed quaternion per face: compute face normal `n` and up direction `u`, solve rotation mapping `(n,u)` onto `(-cameraForward, cameraUp)`. Per-die-type handling for numbering conventions and text uprightness.

**Platform adapters:**

- **iOS:** Obj-C++ shim (`.mm` file) binding to `CAMetalLayer` / `MTKView`. Filament does not take ownership of the layer (use `__bridge` cast). Destroy swap chain before engine. Watch for drawable throttling on `CAMetalLayer`.
- **Android:** JNI shim converting `Surface` -> `ANativeWindow*` via `ANativeWindow_fromSurface`. Release `ANativeWindow` ref when swap chain is destroyed. Call `flushAndWait()` in `onDetachedFromSurface()` before Android destroys the surface.

**Native wrappers** — thin Swift (iOS) and Kotlin/JNI (Android) layers that:
- Provide a native view (`UIView` / `SurfaceView`) hosting the renderer
- Expose `roll(result, duration)` and configuration APIs
- Handle lifecycle (pause/resume/destroy)

**Resource destruction order (critical):** Remove/destroy renderables and entities first -> destroy dependent resources (materials, meshes) -> destroy engine last. Violating this order causes "still in use" crashes.

## Technology Decisions

### Rendering Engine: Filament (Google)

- **License:** Apache 2.0
- **Backends:** Metal (iOS), Vulkan + OpenGL ES 3.0 (Android)
- **Size:** ~2-4 MB per architecture
- **Latest release:** Filament 1.71.0 (April 2026) — actively maintained, Metal backend receives dedicated commits
- **Why:** PBR materials, image-based lighting, and tone mapping out of the box. Mobile-first design. Official Android/Kotlin bindings. Active Google backing.

**iOS Metal status (confirmed April 2026):** Metal is recommended and actively maintained. OpenGL ES on iOS is "not regularly tested" (de facto deprecated). Filament tracks latest Xcode/Apple SDK. Known limitation: ~200ms "first-use stutter" from runtime Metal shader compilation — mitigate by shipping precompiled `.metallib` via `matedit` or warming materials before showing the view.

**iOS lifecycle caution:** Heap corruption reported in the wild when repeatedly opening/closing a Filament view with Obj-C bridge. Design teardown ordering carefully — always destroy in the correct sequence.

Runner-up: bgfx (~1-2 MB, BSD 2-Clause) if binary size is critical and the team can write custom PBR shaders.

### Physics Engine: None

The floating-in-space model is pure quaternion rotation animation. No collisions, no gravity, no surfaces. Zero additional dependencies.

### Predetermined Outcome: Tumble + Correction Quaternion

Integrate angular velocity for natural-looking tumble, then blend in a correction quaternion during the final ~30% of the animation to guarantee the target face ends up toward the camera. Battle-tested approach used by production dice apps.

For implementation details, see [research document](research/filament-cross-platform-dice.md#guaranteeing-the-predetermined-face-tumble--correction-quaternion).

### iOS Bridging: Obj-C++ (not Swift C++ interop)

**Decision: use Obj-C++ shim, not Swift 5.9+ C++ interop.** Rationale:

- Swift C++ interop requires **iOS 16.4 minimum** for imported C++ reference types — too high for a broadly embeddable SDK
- Swift cannot import C++ templates directly (only instantiated specializations via type aliases) — Filament's headers are template-heavy
- Swift does not catch C++ exceptions — uncaught exceptions terminate the program
- `std::function` and `std::variant` are not supported in Swift's C++ bridge
- SwiftPM binary targets (`.binaryTarget`) cannot set `.interoperabilityMode(.Cxx)` — blocks prebuilt XCFramework distribution
- Enabling C++ interop propagates to all dependent targets, which is undesirable for consumer apps
- Filament's own iOS samples use the Obj-C++ pattern

The C++ core exposes a **C ABI** (`extern "C"`) or **Obj-C++ API**. Swift calls Obj-C++. All Filament/C++ types stay private inside the framework.

For full analysis, see [research: Swift C++ interop](research/closing-key-gaps.md#swift-59-c-interop-in-2026-versus-obj-c-bridging).

### Face Number Rendering: Pre-baked Texture Atlases (default)

**Decision: ship pre-baked PNG atlases as default.** Rationale:

- No runtime glyph rasterization code needed
- No platform shims, no font dependencies
- Deterministic results, easiest QA
- Cross-platform consistency (same atlas on iOS and Android)

**Customization via alpha mask tinting:** treat the atlas as an alpha mask and tint die color / number color in the shader. This allows color customization without regenerating the atlas.

**Future path for custom fonts (if needed):**

| Option | Approach | Bundle Size | Quality | Security |
|--------|----------|-------------|---------|----------|
| Platform APIs | Core Text (iOS) / Canvas (Android) via platform shims | Minimal | Best (OS rasterizer) | Safe |
| FreeType (C++) | Single C++ code path, generate atlas at runtime | ~1-2 MB | High | Mature for untrusted input |
| stb_truetype (C++) | Single header, generate atlas at runtime | ~50 KB | Good for large digits | **No security guarantee for untrusted fonts** |

**Recommendation if custom fonts are ever needed:** FreeType in C++ core for a single code path. Fixed atlas size per die (512x512 or 1024x1024) — Filament texture dimensions are immutable after creation.

For full analysis, see [research: text-to-texture](research/closing-key-gaps.md#runtime-text-to-texture-for-dice-face-atlases).

## Mesh Generation Details

### Standard polyhedra (d4, d6, d8, d12, d20)

Well-documented Platonic solid geometry. Can be generated procedurally from known vertex coordinates or loaded as minimal meshes.

### d10 (pentagonal trapezohedron)

12 vertices, 10 kite-shaped quad faces. Two distinct edge lengths: short ~0.618 (`(sqrt(5)-1)/2`), long ~1.618 (`(1+sqrt(5))/2`). This edge-length pattern is a useful debugging invariant.

**Exact coordinates** (from McCooey's polyhedra dataset):

```cpp
const double C0 = (sqrt(5.0) - 1.0) / 4.0;  // 0.309016994...
const double C1 = (1.0 + sqrt(5.0)) / 4.0;   // 0.809016994...
const double C2 = (3.0 + sqrt(5.0)) / 4.0;   // 1.309016994...

// 12 vertices
V0  = ( 0.0,  C0,  C1);   V1  = ( 0.0,  C0, -C1);
V2  = ( 0.0, -C0,  C1);   V3  = ( 0.0, -C0, -C1);
V4  = ( 0.5,  0.5,  0.5); V5  = ( 0.5,  0.5, -0.5);
V6  = (-0.5, -0.5,  0.5); V7  = (-0.5, -0.5, -0.5);
V8  = ( C2,  -C1,  0.0);  V9  = (-C2,   C1,  0.0);
V10 = ( C0,   C1,  0.0);  V11 = (-C0,  -C1,  0.0);

// 10 kite faces (quads) — {apex, a, opposite, b}
F0 = { 8,  2,  6, 11};  F5 = { 9,  0,  4, 10};
F1 = { 8, 11,  7,  3};  F6 = { 9, 10,  5,  1};
F2 = { 8,  3,  1,  5};  F7 = { 9,  1,  3,  7};
F3 = { 8,  5, 10,  4};  F8 = { 9,  7, 11,  6};
F4 = { 8,  4,  0,  2};  F9 = { 9,  6,  2,  0};
```

**Per-face up direction for text:** Each face is `{apex, a, opposite, b}` where apex is vertex 8 or 9.
- `u_raw = opposite - apex` (lies in face plane)
- `u = normalize(u_raw - n * dot(n, u_raw))` (project out normal component)

This gives a repeatable top-to-bottom axis from the pole vertex toward the far kite vertex.

**Winding order enforcement:** Compute face normal from first 3 vertices (right-hand rule), check `dot(normal, centroid) > 0` for outward. Reverse vertex order if negative. Do not rely on source winding.

For full analysis, see [research: d10 mesh](research/closing-key-gaps.md#regular-pentagonal-trapezohedron-d10-mesh-exact-coordinates-faces-winding-and-per-face-up-vectors).

### d16 (octagonal dipyramid)

Equator ring of 8 vertices at `(r*cos(2*pi*k/8), r*sin(2*pi*k/8), 0)` plus two apices at `(0,0,+/-h)`. Top faces: `(top, v_k, v_{k+1})`. Bottom faces: `(bottom, v_{k+1}, v_k)` (reversed winding for outward normals).

### d32 / pentakis dodecahedron

Build a regular dodecahedron (20 vertices, 12 pentagonal faces). For each face, create an apex vertex at face centroid + scaled normal. Triangulate each pentagon into 5 triangles from the apex. Result: 32 vertices, 60 triangles.

**Design decision needed:** 60 faces != 32 outcomes. See open questions.

### UV Mapping

Texture atlas approach: each face gets a cell `(u0..u1, v0..v1)` in the atlas with padding to prevent bleeding during minification. Vertices duplicated per face for independent UV orientation. Numbers rendered near face centroid.

### Tangent Frames

Filament requires `TANGENTS` attribute (as quaternions) for all lit/PBR shading. Use Filament's `SurfaceOrientation` utility rather than writing a custom tangent-quaternion encoder.

## Build System

### Strategy: Filament Prebuilt Binaries + CMake

Use Filament prebuilt binaries per platform (CocoaPods tarballs for iOS, native libs from releases for C++ core). Build core library with CMake per target triple.

**Material pipeline:** Filament compiles materials offline via `matc` (`.mat` -> `.filamat`). Embed compiled material packages in the library — Filament does not compile materials at runtime.

**iOS Metal shader warmup:** To avoid ~200ms first-use stutter, either ship precompiled `.metallib` payloads (via `matedit`) or warm materials before showing the view.

### Deployment Targets

- **iOS minimum: 14.0** — broad 2026 adoption, full Metal support, Filament-compatible, avoids any Swift C++ interop floor concerns
- **Android minimum: API 28** (Android 9 / Pie) — safe Vulkan support, ~95%+ of active Android devices in 2026

### Target Triples

| Platform | Architecture | Toolchain |
|----------|-------------|-----------|
| iOS device | `iphoneos` + `arm64` | Xcode |
| iOS simulator | `iphonesimulator` + `arm64` (+ `x86_64` optional) | Xcode |
| Android | `arm64-v8a`, `armeabi-v7a` (+ `x86_64` optional) | NDK toolchain file |

### Packaging

- **iOS -> XCFramework:** Build static library for device + simulator, then `xcodebuild -create-xcframework`. Consumer must link `c++` standard library and Metal framework. Expose Obj-C++ API (not C++ directly) to avoid Swift C++ interop propagation requirements.
- **Android -> AAR:** JNI `.so` files under `jniLibs/<abi>/`. Use NDK toolchain file (not CMake built-in NDK support). Verify `.so` files merge into consumer APKs — incorrect `jniLibs` placement is a common pitfall.

**Size gotcha:** shipping all architectures for both platforms makes the source distribution large, but per-app footprint remains ~2.5-4.5 MB (single architecture).

### Distribution

**iOS — XCFramework via SPM binary target (primary) + CocoaPods (fallback):**
- SPM binary targets work for XCFrameworks but have limitations: cannot have dependencies on source-based packages, and cannot set `swiftSettings` on binary targets
- Expose Obj-C/Obj-C++ API to avoid C++ interop propagation (`interoperabilityMode(.Cxx)` would cascade to all consumer targets)
- CocoaPods as fallback for consumers with complex dependency graphs or resource packaging needs

**Android — AAR via Maven:**
- AAR naturally includes `jniLibs` native libraries alongside Kotlin/Java wrappers
- Private repos: Gradle `maven-publish` plugin to Nexus/Artifactory
- Maven Central: requires Sonatype Central Portal workflow; no official Gradle plugin exists — use community solutions (JReleaser) or Maven-based publish step in CI

For full analysis, see [research: distribution](research/closing-key-gaps.md#distribution-in-2026-ios-spm-vs-cocoapods-and-android-maven).

## Complexity Assessment

**Overall: 3.5/10** — straightforward. No physics, no networking, no persistence.

| Component | Complexity | Notes |
|-----------|-----------|-------|
| Polyhedra geometry generation | 3/10 | Well-documented math; d16/d32 need custom generation |
| Rendering with PBR materials | 3/10 | Filament handles this out of the box |
| Spin animation (quaternion tumble + correction) | 3/10 | Standard 3D math — angular velocity integration, slerp, easing |
| Face mapping (face -> camera quaternion) | 3/10 | Precompute per die type; d10 uprightness needs care |
| Configurable spin duration | 2/10 | Parameter to animation controller |
| Face number rendering (pre-baked atlas) | 2/10 | Ship PNGs, upload to Filament texture |
| iOS wrapper (Swift + Obj-C++ bridge) | 5/10 | Obj-C++ pattern well-documented; avoid Swift C++ interop |
| Android wrapper (Kotlin + JNI) | 5/10 | Filament has official Kotlin bindings |
| Build system (CMake cross-compile + packaging) | 5/10 | Per-platform builds, XCFramework + AAR packaging |
| Distribution (SPM + Maven) | 4/10 | SPM binary target limitations, Maven Central portal friction |
| Transparent background support | 4/10 | Works on most devices, needs fallback for edge cases |
| Metal shader warmup | 3/10 | Precompile metallib or warm before first render |

### What makes this tractable

- Rendering scope is tiny: one object, one light, empty background
- No physics engine — just quaternion math
- Filament eliminates 80% of rendering complexity (PBR, lighting, shadows)
- Predetermined outcome is trivial without physics to fight
- No networking, no persistence, no complex state management
- Obj-C++ bridging pattern is well-trodden (Filament's own samples use it)

### What could get hard

- **Exotic dice (d16, d32):** Non-standard polyhedra need custom geometry generation and face-mapping. d32 has a face-count mismatch that needs a design decision.
- **Cross-platform build system:** CMake cross-compilation with Filament for iOS + Android takes iteration. Material pipeline (`matc`) and Metal shader precompilation (`matedit`) add build steps.
- **Visual polish:** Materials, lighting, camera angle, animation easing curves — design tuning beyond code.
- **Transparency edge cases:** Filament translucent output can artifact on some Android devices.
- **Filament lifecycle bugs:** iOS heap corruption reported when repeatedly opening/closing views — requires careful teardown ordering and testing.

## Estimated Bundle Impact

| Component | Per-Architecture Size |
|-----------|-----------------------|
| Filament core | ~2-4 MB |
| Dice meshes + texture atlas PNGs | <1 MB |
| Compiled materials (`.filamat`) + metallib | <0.5 MB |
| Native wrapper | <0.5 MB |
| **Total** | **~2.5-4.5 MB** |

## Resolved Decisions

| Question | Decision |
|----------|----------|
| d32 face count | Map 1–32 across 60 faces with repeats (option a) |
| Multiple dice | Yes — required for D&D (2d6, 2d20 advantage, 4d6 stats) |
| Background | Transparent — host composites over own UI |
| Rounded edges | Yes — parametric chamfer, default bevel_factor 0.05 |
| Die color | Configurable RGBA per die instance |
| Number color | Black or white (binary) |
| iOS minimum | 14.0 |
| Android minimum | API 28 (Android 9) |
| Sound effects | Out of scope — host app responsibility |
| Custom die faces (icons) | Out of scope for MVP |
| Gesture interaction (pinch/rotate) | Out of scope for MVP — command-driven only |

## References

- [Deep research: Filament cross-platform dice implementation](research/filament-cross-platform-dice.md) — Filament integration patterns, quaternion animation math, procedural geometry, CMake packaging
- [Deep research: closing key gaps](research/closing-key-gaps.md) — Swift C++ interop analysis, text-to-texture options, Filament iOS status, d10 exact coordinates, distribution paths
