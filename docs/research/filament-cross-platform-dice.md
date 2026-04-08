# Building an embeddable cross-platform C++ 3D RPG dice library for iOS and Android with Filament

## Target behaviour and recommended architecture

The requirements imply a **deterministic orientation controller** rather than physics: the die "floats" in empty space (no table), spins on command, and decelerates to land in a **caller-specified final orientation** where a chosen face is presented to the camera. The practical implication is that you want the library's public API to be *update-driven* (per-frame `tick(dt)`), and to separate concerns into (a) rendering, (b) geometry + face metadata, and (c) rotation animation state. Filament already enforces strong lifecycle and threading constraints (notably that `Engine` is not thread-safe), so it's cleanest to make your library "single render thread" by design. citeturn13search0turn13search10

A common structure for an embeddable native library that must plug into Swift and Kotlin hosts is:

- **C++ core** (`libdice3d_core`): Filament setup, mesh generation, PBR materials, face mapping, quaternion animation, and a small C ABI surface (optional but highly recommended for JNI simplicity).
- **Platform adapters**:
  - iOS: Obj‑C++ shim that holds the C++ core and binds it to `CAMetalLayer` / `MTKView`.
  - Android: JNI shim that binds it to an `ANativeWindow*` (or to Java/Kotlin Filament bindings, if you choose to expose Filament directly on the Java side—but that usually makes your "C++ library" less self-contained).
- **Language-friendly wrappers**:
  - Swift: thin Swift class calling Obj‑C++.
  - Kotlin: thin Kotlin class calling JNI. citeturn13search11turn20search13turn12search7

The key point: your library should **own** the Filament `Engine` and the scene graph, and the app should only provide a native surface ("where to render"), plus input ("roll to face N").

## Embedding Filament in a custom C++ library

### Core Filament object graph and the minimum rendering loop

Filament's own quickstart and examples consistently show the same minimal object set: `Engine`, `SwapChain`, `Renderer`, `View`, `Scene`, and `Camera`. citeturn13search19turn15search3turn13search11

A minimal "render one frame" structure that respects Filament's invariants looks like this:

```cpp
// PSEUDOCODE SHAPE — keep the real code in your core library.
struct RenderContext {
  filament::Engine* engine = nullptr;
  filament::SwapChain* swapChain = nullptr;
  filament::Renderer* renderer = nullptr;
  filament::View* view = nullptr;
  filament::Scene* scene = nullptr;

  filament::Camera* camera = nullptr;
  utils::Entity cameraEntity;

  void renderFrame() {
    if (!swapChain) return;
    if (renderer->beginFrame(swapChain)) {
      renderer->render(view);
      renderer->endFrame();
    }
  }
};
```

Critical gotchas that are easy to miss:

- **Always call `endFrame()` if `beginFrame()` returns `true`**, even if you skip drawing for the frame. This is explicitly called out in the `Renderer` API docs. citeturn13search1
- Filament's `Engine` is **not thread-safe**, so create a single "render thread affinity" for *all* Filament calls unless you externally synchronise everything. citeturn13search0turn13search10
- When a native surface is destroyed or changed, destroy and recreate the `SwapChain`. Filament's `SwapChain` docs include explicit "destroy and recreate on resize / recreation" guidance. citeturn13search11

### iOS integration pattern (Metal)

On iOS, Filament expects a pointer to a `CAMetalLayer*` (Metal) or `CAEAGLLayer*` (OpenGL ES) when creating the swap chain. Filament's repo and iOS sample code show `CAMetalLayer* metalLayer = (CAMetalLayer*) view.layer;` passed to `Engine::createSwapChain`. citeturn13search11turn21search2turn21search1

A concrete and battle-tested pattern is:

- Use an `MTKView` (or `UIView` backed by a `CAMetalLayer`) supplied by the host app.
- In Obj‑C++, grab `CAMetalLayer*`, create the Filament `Engine` with Metal backend, then `createSwapChain(layer)`.

The Filament iOS CocoaPods sample (`ViewController.mm`) highlights an important ownership rule: **Filament does not take ownership of `CAMetalLayer`**, and they use bridging casts rather than transferring ownership. citeturn21search2

Practical iOS pitfalls:

- **Drawable throttling**: `CAMetalLayer` has a finite drawable pool; if your rendering loop or resource handling accidentally "holds on" to drawables too long, calls that acquire drawables can block. While Filament abstracts this, your host app should still avoid doing heavy work on the render thread and should not block the per-frame loop. citeturn15search10
- **Always detach the surface before destroying the engine**: this is a pattern also emphasised in Filament's Android `UiHelper` sample, and it applies conceptually on iOS as well (destroy swap chain first, then engine). citeturn20search6turn13search9

### Android integration pattern (Vulkan / OpenGL ES)

Filament supports multiple backends (OpenGL ES, Vulkan, Metal, etc.) and documents the backend enum; on Android you'll typically allow `DEFAULT` or choose Vulkan / OpenGL ES depending on device support and your own testing matrix. citeturn13search2

For a "pure C++ core" that renders into an Android `Surface`, the canonical NDK flow is:

1. In JNI, convert a Java `Surface` to `ANativeWindow*` via `ANativeWindow_fromSurface`.
2. Pass that pointer to `engine->createSwapChain(win)`.

Filament's `SwapChain` docs include the exact snippet and header (`<android/native_window_jni.h>`). citeturn13search11

Memory/lifecycle gotcha: NDK windows are refcounted; the NDK docs specify that you must release acquired references with `ANativeWindow_release`. Your JNI should retain the window pointer only as long as the swap chain is valid, and release it once Filament no longer needs it (often immediately after creating the swap chain if Filament makes its own internal ref; otherwise at swap chain destroy time—test this explicitly). citeturn20search13turn13search11

Surface lifecycle gotcha: Filament's own Android `UiHelper` includes a very specific warning—after `destroySwapChain`, you should call `flushAndWait()` in `onDetachedFromSurface()` to ensure Filament has executed the destroy command before Android destroys the `Surface`. citeturn20search6turn13search4turn13search9

### Resource destruction ordering and "still in use" crashes

Filament has strict lifetime rules: destroying a `MaterialInstance` (or other resource) that is still referenced by a renderable can trip preconditions / abort. Real-world crash reports show this exact pattern ("destroying MaterialInstance … still in use by Renderable"). The fix is always the same: remove/destroy renderables (and entities) first, then destroy dependent resources, then the engine last. citeturn20search14turn11search1turn11search13

### Transparent "empty space" background

Your requirement says "empty space", which in UI terms often means **transparent** so the host app can composite it over its own background.

Filament provides a swap chain transparent config flag (`CONFIG_TRANSPARENT`) in its `SwapChain` docs, and the `View` API exposes an output blend mode (`OPAQUE` vs `TRANSLUCENT`). Both typically need to be set correctly, *and* the underlying platform view must support alpha compositing. citeturn13search11turn23search0

Important gotchas to plan for:

- **Filament transparency can interact badly with dynamic resolution or other features** in some cases; there are documented issues where setting `View::BlendMode::TRANSLUCENT` produced pixelated / torn output or other artefacts, especially on some Android devices. Treat translucent output as a feature that needs device testing and a fallback path. citeturn23search4turn23search10
- Clearing with alpha (`Renderer::setClearOptions`) in itself does not guarantee you'll get a transparent composited result; you still need a transparent swap chain and correct view blending, and the host surface must actually have something behind it to see through. The renderer clear options API shows how clearing is configured. citeturn23search13turn23search5
- On Android, `SurfaceView` compositing is special (it's a separate surface layer). Android's own docs note that `SurfaceView` compositing depends on layout and view hierarchy, and transparency can be counterintuitive. Plan to support both `SurfaceView` and `TextureView` hosts, and document trade-offs. citeturn22search4turn22search33

## Quaternion-based spin animation with deterministic landing

### Quaternion integration from angular velocity

A physically plausible "tumble" feel comes from integrating angular velocity over time rather than directly slerping between start and end (a plain slerp tends to look like "rotate around one axis"). Quaternion rotation animation in graphics is classically treated in Shoemake's work, and common formulations relate quaternion derivative to angular velocity. citeturn8search0turn8search32turn8search2

A robust real-time pattern (stable and easy to implement) is to use **axis–angle incremental updates**:

1. Keep an angular velocity vector **ω** (radians/sec) in world or local space.
2. Each frame, build a delta quaternion from `axis = normalise(ω)` and `angle = |ω| * dt`.
3. Multiply current orientation by delta and renormalise.

This incremental approach aligns with widely used "finite rotation each timestep" reasoning for quaternion integration. citeturn8search6turn8search17turn8search2

### Natural-looking tumble: randomised multi-axis angular velocity and controlled damping

A minimal "floating dice spin" (no gravity) usually looks best if:

- The initial angular speed is high and multi-axis (not aligned to camera axes).
- The angular speed decays smoothly to zero (no abrupt stop).
- The axis can *slightly wander* (optional) so it doesn't feel like a perfectly rigid motor spin.

Easing functions are the practical tool for the decay curve. Robert Penner's easing equations (widely reused across animation systems) provide standard "ease out" families, and easings.net provides an intuition-oriented catalogue. citeturn8search3turn8search7

A simple and effective model is:

- Normalised time `t = clamp(elapsed / duration, 0..1)`
- Angular speed multiplier `s(t) = 1 - easeOutQuint(t)` (or `1 - easeOutCubic(t)`)
- `ω(t) = ω0 * s(t)`

For "floating dice", these three tend to read naturally:

- `easeOutCubic`: smooth deceleration without feeling "draggy". citeturn8search3turn8search7  
- `easeOutQuint`: stronger "fast start, long tail" that feels airy / inertial. citeturn8search3turn8search18  
- `easeOutExpo`: very punchy start that can look good for short spins but risks looking "gamey" if overused. citeturn8search3turn8search7  

### Guaranteeing the predetermined face: tumble + correction quaternion

You want the die to feel like it tumbles freely, but also end **exactly** at a caller-specified face orientation. A practical way to do this without physics is to decouple "style motion" from "final constraint":

1. Generate a tumble trajectory `q_tumble(t)` by integrating angular velocity.
2. Compute where the tumble would end: `q_raw_end = q_tumble(T)`.
3. Compute a correction quaternion: `q_corr = inverse(q_raw_end) * q_target`.
4. Blend in the correction over time, so early motion is free and late motion converges.

A strong pattern is:

- `blend(t)` is near 0 for the first ~60–80% of the duration, then rises smoothly to 1.
- Final orientation is exact because at `t = T` you apply the full correction.

Shoemake's quaternion curve work is the canonical reference for using slerp and quaternion interpolation for animation, and this approach is effectively "tumble + slerp correction". citeturn8search0turn8search8

Two key gotchas:

- **Shortest-path slerp**: if `dot(qA, qB) < 0`, flip one quaternion (`qB = -qB`) to avoid spinning the "long way around". This is standard practice in quaternion interpolation literature and implementations. citeturn8search0turn8search2
- **Choosing correction timing**: If correction starts too early, the roll looks like "steering" rather than tumbling. If it starts too late, you can get a visible snap. Using an ease-in (or ease-in-out) for the correction blend, and starting it around 70% of the animation, tends to work well in UI dice. citeturn8search7turn8search3

## Procedural generation of d16 and "d32" geometry with UV mapping

### d16 as an octagonal dipyramid

A dipyramid (bipyramid) is formed by joining two pyramids base-to-base; an *n*-gonal bipyramid has **2n triangular faces** (so *n* = 8 gives 16 faces) and has the characteristic "two apices + equator ring" topology. citeturn10search0

A clean procedural mesh parameterisation:

- Choose equator radius `r` and apex height `h`.
- Equator vertices for k = 0..7:
  - `v_k = (r * cos(2πk/8), r * sin(2πk/8), 0)`
  - This is the standard regular n‑gon construction by rotation. citeturn9search24
- Apices: `top = (0,0,h)`, `bottom = (0,0,-h)`

Faces (triangles):

- Top half: `(top, v_k, v_{k+1})`
- Bottom half: `(bottom, v_{k+1}, v_k)` (reverse winding so normals point outward)

Because dice texturing almost always requires seams per face, you generally **duplicate vertices per face** (each triangle has its own 3 vertices) so each face has independent UVs and a clean tangent frame. This aligns with Filament's expectation that geometry is provided via `VertexBuffer` with attributes like `POSITION`, `UV0`, and `TANGENTS`. citeturn11search0turn11search4

### "d32" as a pentakis dodecahedron: face count mismatch and what it implies

A **pentakis dodecahedron** is a Catalan solid obtained by attaching a pentagonal pyramid to each face of a regular dodecahedron. It has **60 triangular faces** and **32 vertices**. This is a common source of confusion: it is "32" by vertex count, not by face count. citeturn9search1turn9search26turn9search23

If you truly need 32 outcomes labelled 1–32, you have three options:

- Accept that the solid has 60 faces and map outcomes onto faces (some numbers repeat).
- Treat it as a d60 (more consistent with its 60 faces).
- Use a different polyhedron with 32 faces (not covered here because your prompt explicitly names pentakis dodecahedron). citeturn9search1turn9search23

### Pentakis dodecahedron procedural construction that avoids convex hull code

You can construct the mesh without running a convex hull algorithm by using its definition:

1. Build a regular dodecahedron (20 vertices, 12 pentagonal faces).
2. For each pentagonal face, create an apex point and triangulate the pyramid into 5 triangles.

The pentakis dodecahedron page provides Cartesian coordinates for a canonical version using the golden ratio φ, and describes a construction as a dodecahedron combined with a scaled icosahedron (the icosahedron vertices correspond to face normals of the dodecahedron in the dual relationship). citeturn9search1turn9search10

The high-leverage engineering trick is:

- Hardcode the **dodecahedron face index list** once (12 faces × 5 vertex indices).
- Generate the 12 apex vertices either:
  - by using face centroids and normals (pick a height that looks good), or
  - by projecting to the canonical scaled-icosahedron radius described in the reference coordinates for a mathematically standard pentakis dodecahedron. citeturn9search1

This yields:
- Vertices: 20 (dodeca) + 12 (apexes) = 32
- Triangles: 12 faces × 5 triangles each = 60 triangles citeturn9search1turn9search23

### UV mapping for face numbers (triangles and pentagons)

For dice, the most robust UV approach is an **atlas**:

- Prepare a texture atlas that contains all face labels (e.g., 1–20, 0/10/… for d10, etc.).
- Assign each face a cell `(u0..u1, v0..v1)` in the atlas.
- For each face, map its polygon into UV space inside that cell with padding to prevent bleeding.

Implementation tips that matter in practice:

- Duplicate vertices per face so each face can have unique UV orientation without affecting neighbours. This is standard for "seamed" hard-surface props like dice. citeturn11search0turn11search4
- For **triangular faces**, map triangle vertices to a consistent UV triangle inside the atlas cell. Put the number near the triangle centroid; keep generous padding because minification filtering will sample neighbouring cells.
- For **pentagonal faces** (d12 base before "kissing"), either:
  - triangulate the pentagon and assign UVs for each triangle (still fine), or
  - duplicate the face into a "fan" from the face centre so UV orientation is easy to keep consistent with text "uprightness".

Filament-specific constraint: if you use the lit / PBR shading model, you will need `TANGENTS` (Filament requires tangents for all shading models except `unlit`). citeturn11search20turn11search4

### Tangent frame generation for Filament (procedural meshes)

Filament stores tangent frames as **quaternions** in the `TANGENTS` attribute, and Filament maintainers explicitly note two practical tools:

- `SurfaceOrientation` (a utility for generating tangents), used in dynamic geometry pipelines. citeturn24search3turn24search5  
- A helper that computes a quaternion tangent frame from a TBN matrix (noted by Filament team in a "compute tangents" utility discussion), which is intended for simple use cases. citeturn24search2

In short: for procedural dice meshes, prefer using Filament's provided tangent utilities rather than writing your own tangent-quaternion encoder unless you already have a tangent-space pipeline.

## Face mapping and precomputed "face-to-camera" quaternions

### General face-to-camera quaternion derivation

To deterministically "land" a particular face toward the camera, you need a **mapping from face identifier → target orientation quaternion**.

A robust general approach (works for any convex polyhedron mesh):

1. For each face, compute:
   - face normal `n` (outward unit normal)
   - face "up" direction `u` (unit vector in face plane that corresponds to "top of the printed number")
2. Define camera basis in world space:
   - desired face normal direction `N = -cameraForward` (or `cameraToObject` depending on your convention)
   - desired "up" direction `U = cameraUp`
3. Solve for a rotation `q_face` that maps `(n,u)` onto `(N,U)`.

This is most robustly done by building orthonormal bases and converting a rotation matrix to a quaternion (or by "align normal then twist about normal"). This is the same conceptual operation as the transform hierarchy operations Filament's `TransformManager` expects, because you ultimately set an entity transform matrix derived from a quaternion. citeturn19search5turn11search4

### Per-die notes that matter for numbering and uprightness

Even if you implement the general algorithm above, each die type has "numbering conventions" and "uprightness" details.

- d6: faces are axis-aligned; you can precompute 6 quaternions directly from ±X/±Y/±Z normals, plus a twist to ensure the number is upright. (Use the mesh-derived method anyway so you don't bake coordinate assumptions.) citeturn11search4turn19search5
- d10 (pentagonal trapezohedron): text uprightness is tricky because faces are kites rather than regular polygons; you must define a consistent `u` per face from the geometry (e.g., direction from face centroid toward the "top" vertex or a designated edge). A practical reference implementation in another ecosystem explicitly calls out custom geometry and settle quaternions for d10 and other dice. citeturn17search0
- d16 octagonal dipyramid: all faces are triangles; defining `u` as "project world up onto face plane, then normalise" works well visually, but you must handle the degeneracy when face normal is near world up/down (choose a fallback axis). The dipyramid topology supports this well because no face normal is exactly aligned with the symmetry axis unless you distort it heavily. citeturn10search0turn9search24
- Pentakis dodecahedron: 60 triangular faces; if you label 1–32 across faces (with repeats), you'll need `number → list<face>` mapping, then select a face instance deterministically (or randomly) when asked to land on "number k". The polyhedron's 60-face property is explicit in references, so this is not just theoretical. citeturn9search1turn9search23

### Applying the quaternion to the Filament renderable

Filament uses an entity-component model; transforms are manipulated via `TransformManager::setTransform` using matrices (column-major), and the `TransformManager` docs show typical usage. citeturn19search5turn19search10

Practically:

- Keep the die renderable as a single entity (or a small hierarchy if you separate decals/labels).
- Build a `mat4f` from translation + quaternion rotation each frame.
- Call `setTransform(instance, mat4f)` inside the render thread loop. citeturn19search5turn13search0

## CMake cross-compilation and packaging into XCFramework and AAR

### Building with Filament as a dependency: prebuilt vs source

Filament supports iOS and Android and is designed to be small and efficient on mobile. citeturn11search2turn13search2

For an embeddable library, you have two realistic strategies for Filament integration:

- **Use Filament prebuilt binaries per platform**  
  - iOS: Filament ships CocoaPods specs that fetch prebuilt iOS tarballs from Filament releases (`filament-…-ios.tgz`) and set platform minimums (e.g., iOS 11.0). This is strong evidence that Filament's own maintainers treat "prebuilt iOS binaries" as a supported distribution form. citeturn21search3turn21search1  
  - Android: Filament is published on Maven (e.g., `com.google.android.filament:filament-android`), which can be used from Kotlin. If your core is native C++, you may still prefer native libs built from source to avoid "Java-first" dependency structure. citeturn13search26turn21search14  

- **Build Filament from source in a superbuild**  
  - Filament's build script supports platform builds (including iOS and Android), and it is common to build host tools plus target libs. citeturn21search4turn21search8  
  - This gives you maximum control but increases CI time and complexity.

A critical Filament pipeline constraint: Filament aims not to compile materials at runtime; instead it provides `matc` to compile material packages offline. This affects your build because you should generally compile `.mat` → `.filamat` as part of your asset pipeline and embed the binary package into your library. citeturn21search9turn15search3

### CMake presets for iOS and Android

For iOS and Android you rarely want a single "one shot" CMake invocation; instead use presets (or scripts) that produce one build per target triple:

- iOS device: `iphoneos` + `arm64`
- iOS simulator: `iphonesimulator` + (`arm64` and/or `x86_64` depending on your support goals)
- Android ABIs: at least `arm64-v8a` and `armeabi-v7a` (optionally `x86_64`) citeturn12search7turn21search5

Android-specific CMake gotcha: Android's NDK documentation warns about differences between CMake's built-in NDK support and the NDK toolchain file, and recommends using the NDK toolchain workflow (which Gradle's `externalNativeBuild` does automatically). citeturn12search7

### Producing an XCFramework for iOS

Apple's documentation for distributing binary frameworks is centred on XCFramework bundles, and they describe creating a multi-platform binary framework bundle. citeturn12search8turn12search5

Practical pattern:

1. Build your static library for device and simulator as separate outputs.
2. Run `xcodebuild -create-xcframework -library ... -headers ...`.

Apple's WWDC guidance also outlines the archive → create-xcframework flow. citeturn12search14turn12search1

Filament-specific iOS gotcha: if you link Filament statically into your XCFramework, ensure the consumer links the correct C++ standard library and any system frameworks required by Metal usage. Filament's own podspec explicitly adds `c++` to linked libraries, signalling this is a normal requirement for consumers. citeturn21search3

### Producing an AAR for Android

For Android, the normal packaging unit is an **AAR**. If your library has JNI `.so` outputs, you package them under `jniLibs/<abi>/libYourLib.so`. Android documentation covers CMake use with the NDK and native dependency packaging patterns (including AAR-based distribution). citeturn12search7turn12search16turn12search28

Practical gotchas:

- Make sure your AAR's `.so` files are actually merged into consumer APKs; historically there have been build and merge pitfalls, and many real-world issues reduce to incorrect `jniLibs` placement or ABI filtering. citeturn12search6turn12search12
- If you decide to ship Filament native binaries inside your AAR, be mindful of artifact size; projects that ship *all* architectures for iOS + Android can become very large in source distribution even if the final app footprint is small. One prominent Filament-wrapper project notes that its package is huge because it ships static libs for all architectures, even though the integrated app size impact is much smaller. citeturn15search4turn15search5

## Open-source projects worth studying for architecture patterns

Although many dice rollers rely on physics engines, several open projects are still highly relevant for your specific "spin then settle to predetermined face" and for mobile-native packaging patterns:

- **A cross-platform Filament wrapper with extensive native packaging**: a Filament-based rendering library that ships native code and supports iOS and Android demonstrates real-world strategies for bundling many architectures and dealing with mobile build systems. Its documentation describes Metal/OpenGL/Vulkan support and highlights the cost of shipping multi-arch binaries. citeturn15search5turn15search4turn15search6  
- **A dice renderer that explicitly computes settle quaternions**: a small 3D dice renderer project in another ecosystem documents functions like `settleQuat`, face extraction, and a "settle animation uses quaternion slerp with cubic ease-out". Even if not C++, its algorithmic breakdown is directly portable to your quaternion-only deterministic landing problem. citeturn17search0turn8search3  
- **Native iOS SceneKit dice demos**: open iOS dice animation demos are useful for "what looks good" heuristics and UI integration patterns, even if you do not use SceneKit. They often implement "show faces in random order then settle" style animation that conceptually matches your non-physics brief. citeturn16view1turn16view0  

The practical takeaway from studying these is less about copying code and more about copying *interfaces*: keep the C++ core authoritative, keep platform wrappers thin, and make lifecycle edges (surface created/destroyed/resized; app backgrounded/resumed) explicit in your API so host apps cannot misuse the engine. citeturn20search6turn13search0turn13search11
