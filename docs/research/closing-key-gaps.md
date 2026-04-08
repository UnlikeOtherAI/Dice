# Closing key gaps for a cross-platform C++ 3D dice library using Filament on iOS and Android

## Swift 5.9+ C++ interop in 2026 versus Obj-C++ bridging

Swift **can** call many C++ APIs directly (including classes) without an Obj‑C++ layer, provided you enable C++ interoperability in the Swift target. Swift's official C++ interop documentation describes this as introduced in Swift 5.9 and evolving across subsequent releases. citeturn1view1turn2view0turn0search4turn0search20

However, there are several **hard constraints** that matter specifically for a Filament-backed XCFramework SDK:

Swift's C++ interop is not "all of C++"
Swift's supported surface is a subset, and the missing pieces line up uncomfortably well with "graphics engine style" headers (heavy templates, inline APIs, ownership helpers, etc.):

- **R-value references / universal references**: Swift rejects functions / constructors that use r-value reference types (including many perfect-forwarding patterns). citeturn2view0  
- **Templates**: Swift does *not* import class / struct templates directly; only instantiated specialisations are usable, typically surfaced via **type aliases** in headers. Function templates are supported only with significant restrictions (no dependent types in the signature, no non-type template parameters, no variadic templates, etc.). citeturn2view0  
- **Namespaces**: namespaces are fine (this part is good for modern C++ libraries). citeturn2view2  
- **C++ standard library support**: Swift (as of the current status page) supports specialisations of `std::shared_ptr`, `std::unique_ptr`, `std::vector`, `std::map`, `std::optional`, etc., but *not* `std::function` or `std::variant` (both common in modern C++ APIs). citeturn2view1  
- **Exceptions**: Swift does not catch C++ exceptions; if an uncaught C++ exception reaches Swift, Swift terminates the program. For a shipped SDK, this effectively means you must treat your Swift-facing boundary as `noexcept` (catch in C++ and translate to error codes / Swift error results). citeturn2view3  

A very practical iOS deployment constraint (often a deal-breaker)
Swift's status page contains a **minimum deployment version table** for using imported C++ types that become **reference types** in Swift. For iOS, the table indicates **iOS 16.4** as the minimum deployment version in that scenario. citeturn1view0  

For an SDK intended to be widely embeddable, an **iOS 16.4 floor** can be unacceptable. This is one of the biggest reasons an Obj‑C++ (or C) boundary remains common in production SDKs: Swift can call C / Obj‑C APIs without pulling in these C++-interop deployment constraints.

SwiftPM build-system friction for *binary* XCFramework distribution
If you plan to ship a **prebuilt** XCFramework and let client apps "just import it," you quickly run into the SwiftPM impedance mismatch:

- Enabling C++ interop in a Swift package target is done via `swiftSettings: [.interoperabilityMode(.Cxx)]`. citeturn13view0turn14search17  
- But **binary targets** (`.binaryTarget`) do **not** expose `swiftSettings`, so you cannot directly enable `.interoperabilityMode(.Cxx)` "on the binary." This is explicitly raised as a blocker by developers trying to use static binaries + C++ interop in SwiftPM. citeturn14search2  
- Additionally, Swift's own "project build setup" guide states that if a target enables C++ interop, other targets that depend on it will need to enable C++ interop as well—creating a propagation requirement that many teams dislike for consumer-facing SDKs. citeturn13view0turn14search1turn14search20  

Implication for a Filament backed C++ dice XCFramework
Filament is a modern C++ library with deep namespaces and a lot of template-heavy infrastructure. Even if Swift *can* import some of it, you should expect to end up writing a **Swift-friendly C++ façade** anyway (no templates in public headers, no exceptions crossing the boundary, minimal overload sets, plain value types). The Swift status page's template constraints and exception rule are the core drivers here. citeturn2view0turn2view3  

Why Obj‑C++ is still the safer bet for 2026 distribution
Obj‑C++ remains the lowest-risk approach when you want:
- broad iOS deployment targets without C++ interop constraints (e.g., below iOS 16.4),
- simple Swift integration (no `.interoperabilityMode(.Cxx)` propagation),
- the ability to hide Filament and your internal C++ types completely.  

Filament's own iOS sample code demonstrates this pattern: an Objective‑C++ `.mm` file owns Filament objects and bridges a `CAMetalLayer` into `Engine::createSwapChain`, explicitly noting Filament does **not** take ownership of the layer (hence `__bridge`). citeturn8search15  

Official Apple guidance on Swift+C++ still positions it as an incremental migration / mixing story (WWDC23 "Mix Swift and C++"), not specifically as "the default SDK shipping interface style." citeturn14search32turn13view0  

## Runtime text-to-texture for dice face atlases

You need a way to get *numbers onto faces*. In a Filament-based renderer, the final output is usually: CPU generates or loads an **RGBA atlas**, you upload it into a Filament `Texture`, then your material samples that atlas and uses per-face UVs.

Filament runtime texture updates: what the API really allows
Filament exposes `Texture::setImage(...)` (and async variants) to upload image data to a texture level, and supports sub-image updates for arrays / 3D / cubemap-like layouts. citeturn4search1  
A key constraint is that **texture dimensions are fixed** after creation; if you need a different atlas size, you must destroy and recreate the `Texture`. Filament maintainers explicitly state this in discussions about runtime texture reuse. citeturn4search27  

This usually pushes you toward choosing a **fixed atlas size per die** (e.g., 512×512 or 1024×1024) and keeping it stable.

Option A: pre-bake PNG atlases (most robust SDK choice)
This is the simplest operationally:
- You ship a texture atlas (or a few atlases) as assets.
- No runtime glyph rasterisation code.
- No platform shims.
- Deterministic results, easiest QA.

The trade-off is customisation: allowing user-provided fonts or colours becomes awkward. You can regain some customisability by separating "ink" from "background" and tinting in the shader (e.g., treat the atlas as an alpha mask), but that constrains visual style.

For a cross-platform SDK with minimal integration friction, **pre-baked atlases** are usually the default, and you only add runtime text if you truly need custom fonts/colours.

Option B: platform text APIs via thin shims (best typography, most platform glue)
This means: iOS uses Core Text/Core Graphics; Android uses Canvas/Paint.

iOS path (Core Graphics bitmap context + Core Text)
A standard pipeline is:
1. Create an RGBA bitmap context (`CGBitmapContextCreate` or the modern `CGContext` bitmap initializer).
2. Draw text into it using Core Text. Apple documents `CTLine` as a glyph-run container that can draw itself, and `CTLineDraw` as the convenience drawing function. citeturn3search16turn3search12turn4search10turn4search7  
3. Extract pixels and upload to Filament via `Texture::setImage` + `PixelBufferDescriptor`. citeturn4search1  

Core Graphics also documents typical bitmap context creation parameters (bytes per row, colour space, premultiplied alpha) and shows the "create bitmap context → draw → get data → free" lifecycle. citeturn4search10turn4search32  

Android path (Bitmap + Canvas.drawText)
Android's official API reference shows `Canvas.drawText(...)` overloads (present since API level 1) and the role of `Paint` for text attributes such as colour and size. citeturn4search22turn4search4  

A typical pipeline is:
1. Allocate a `Bitmap` in `ARGB_8888`.
2. Create a `Canvas(bitmap)`.
3. Configure `Paint`/`TextPaint` (anti-aliasing, font size, typeface).
4. `canvas.drawText(...)`.
5. Read pixels into a direct buffer and pass into your C++ core, then upload with Filament `setImage`. citeturn4search22turn4search1  

Trade-offs for platform shims
- **Bundle size**: minimal additional native code, but you may still ship fonts as assets and load them using platform APIs.
- **Quality**: best in class (you get the OS font rasteriser, hinting, shaping, etc.).
- **Customisability**: excellent (arbitrary fonts/colours; high DPI; locale-aware digits if needed).
- **Complexity**: platform-specific glue and threading/lifecycle concerns (especially if you want this entirely from C++).

Option C: C++ font rasterisation (single core codebase; best for "custom fonts everywhere")
Two common choices for a lightweight engine SDK are stb_truetype vs FreeType.

stb_truetype
- `stb_truetype.h` is a single-file TrueType rasteriser widely used in games/tools.
- It is explicitly identified as "public domain" and includes APIs for baking bitmaps and packing glyphs. citeturn17view0turn17view3  
- It also includes a very explicit warning: **"NO SECURITY GUARANTEE — DO NOT USE THIS ON UNTRUSTED FONT FILES"**, because it does no range checking and could allow arbitrary memory reads with malicious fonts. For an SDK, this matters if you let users feed arbitrary TTF/OTF data. citeturn17view0  

FreeType
- The FreeType project describes itself as a portable C library designed to be small/efficient and capable of high-quality glyph output across many font formats. citeturn3search3  
- Android's AOSP also includes FreeType under the FreeType project license, reflecting how standard this library is in production toolchains. citeturn3search28  

Trade-offs for a pure C++ path
- **Bundle size**: stb_truetype is typically smallest in source footprint (single header); FreeType is larger but still widely considered "small and portable" for a font engine and often acceptable for SDK distribution. citeturn17view0turn3search3  
- **Quality**: FreeType generally wins, especially for small point sizes and hinting; stb can be "good enough" for large digits. (For dice faces, digits are usually large, which favours stb.)
- **Customisability**: both allow arbitrary fonts/colours (you rasterise into your atlas).
- **Security**: stb's warning makes it a poor choice if you accept untrusted fonts; FreeType is more mature for adversarial input, but you should still treat font files as untrusted and consider sandboxing or strict validation if user-provided. citeturn17view0turn3search3  

A concrete recommendation for dice faces
- If you do **not** require user-provided fonts: ship **pre-baked atlases** to minimise risk and integration friction.
- If you require user-provided fonts/colours and want a single code path: use **FreeType** in the C++ core for quality and robustness, and generate a fixed-size atlas once at runtime, updating via Filament `Texture::setImage`. citeturn3search3turn4search1turn4search27  
- If you require user-provided fonts but will only accept a trusted bundled font: stb_truetype is viable, but do not accept arbitrary font bytes through your API unless you are comfortable with its explicit security caveat. citeturn17view0  

## Filament iOS and Metal maintenance status as of early April 2026

Filament's public repository shows extremely recent activity and releases, consistent with active maintenance. The GitHub releases page lists **Filament 1.71.0** with a timestamp of **8 April 2026** (today's date in your request context), and the commit history includes "Release Filament 1.71.0" on the same date. citeturn6view0turn6view2  

iOS backend status and official positioning
Filament's iOS samples README states:
- both OpenGL ES 3.0 and Metal backends are supported,
- Metal is recommended,
- Filament is kept up-to-date with Apple's latest SDK and must be built with the latest Xcode,
- OpenGL ES on iOS "is not regularly tested." citeturn1view2turn5search27  

This is both a maintenance signal (they claim to track latest Apple SDK) and a *de facto* deprecation signal for iOS OpenGL ES (supported but not regularly tested). citeturn1view2  

Build and toolchain support
Filament's BUILDING.md explicitly calls out iOS builds via `build.sh -p ios` as the easiest route, and points to the iOS samples README. citeturn8search11turn1view2  

iOS/Metal-specific practical limitations that show up in real projects
Even with active maintenance, several iOS/Metal realities matter for a shipped SDK:

- "First-use stutter" from runtime shader compilation: An iOS user reported noticeable (~200ms) stutter the first time materials are used due to Metal shader compilation. A Filament maintainer replied that **precompiling Metal shaders** is "good low-hanging fruit" and that embedding precompiled `.metallib` via `matedit` is supported (and can also apply to post-processing shaders). citeturn6view4turn8search3  
  For a dice library, this suggests your SDK should either:
  - keep materials extremely simple, or
  - compile/warm critical materials before showing the view, or
  - ship precompiled metallib payloads for your materials if you want ultra-smooth first render. citeturn6view4  

- Toolchain deprecations can break source builds: there is a Metal-backend build issue report (June 2025) describing failures under Xcode 15.x / newer SDKs due to deprecated Metal pixel formats treated as errors because of `-Werror`. This is a reminder that "build from source" consumers will occasionally hit Xcode churn unless they track Filament releases carefully. citeturn8search6turn5search4  

- Lifecycle stability issues exist in the wild: for example, an iOS/Metal issue report describes "heap corruption" instability when repeatedly opening/closing a Filament view and loading/unloading glTF assets, in an integration using a SwiftUI view via Obj‑C bridge. Even if such bugs are not universal, it underscores that iOS integrations must be careful about teardown ordering and resource lifetime. citeturn8search2  

Evidence of active Metal backend work (not just general repo activity)
In addition to the April 2026 releases, Filament's CI / actions feed includes Metal-labelled changes (e.g., a run titled "metal: Clamp minimum stencil texture size…" in March 2026), which supports the claim that Metal backend code is not dormant. citeturn8search25  

Bottom line for your dice SDK decision
As of early April 2026, Filament iOS/Metal appears actively released and maintained, but you should still design your SDK to:
- hide Filament behind a stable boundary (so you can upgrade Filament without breaking consumers),
- plan for shader warmup / precompile strategies on iOS Metal,
- test against "latest Xcode" continually, because Filament explicitly expects that. citeturn1view2turn6view0turn6view4  

## Regular pentagonal trapezohedron (d10) mesh: exact coordinates, faces, winding, and per-face "up" vectors

If you want a procedural d10 suitable for UV-per-face texturing, it is highly practical to start from a canonical set of coordinates + face indices used in polyhedra tooling.

A concrete, exact vertex + face list
David McCooey's dataset provides a compact coordinate set for the **Pentagonal Trapezohedron** including:
- three exact constants expressed using `sqrt(5)`,
- 12 vertices `V0..V11`,
- 10 kite faces as quads with indices. citeturn12view0  

Below is the data in a copy/paste friendly form (exactly as published there, with minor whitespace formatting). citeturn12view0

```cpp
// Pentagonal Trapezohedron (d10) coordinate set
// Constants:
const double C0 = (sqrt(5.0) - 1.0) / 4.0; // 0.3090169943749474...
const double C1 = (1.0 + sqrt(5.0)) / 4.0; // 0.8090169943749474...
const double C2 = (3.0 + sqrt(5.0)) / 4.0; // 1.3090169943749474...

// Vertices V0..V11:
V0  = ( 0.0,  C0,  C1);
V1  = ( 0.0,  C0, -C1);
V2  = ( 0.0, -C0,  C1);
V3  = ( 0.0, -C0, -C1);
V4  = ( 0.5,  0.5,  0.5);
V5  = ( 0.5,  0.5, -0.5);
V6  = (-0.5, -0.5,  0.5);
V7  = (-0.5, -0.5, -0.5);
V8  = ( C2,  -C1,  0.0);
V9  = (-C2,   C1,  0.0);
V10 = ( C0,   C1,  0.0);
V11 = (-C0,  -C1,  0.0);

// Faces (10 kites, each as a quad of vertex indices):
F0 = { 8,  2,  6, 11 };
F1 = { 8, 11,  7,  3 };
F2 = { 8,  3,  1,  5 };
F3 = { 8,  5, 10,  4 };
F4 = { 8,  4,  0,  2 };
F5 = { 9,  0,  4, 10 };
F6 = { 9, 10,  5,  1 };
F7 = { 9,  1,  3,  7 };
F8 = { 9,  7, 11,  6 };
F9 = { 9,  6,  2,  0 };
```

Kite-face geometry and edge lengths
A distinguishing property of a "regular" pentagonal trapezohedron is that it has **two distinct edge lengths** (10 short, 10 long). McCooey's metrics page gives exact expressions:
- short edge: \((\sqrt 5 - 1)/2 \approx 0.6180\)
- long edge: \((1+\sqrt 5)/2 \approx 1.6180\) citeturn10view1  

This is highly useful for debugging: if your generated mesh doesn't show this edge-length pattern, you are not building the same canonical solid.

Winding order and outward normals (what to do in code)
Different sources sometimes list face indices with varying winding. The safest approach is to enforce outward winding in your own pipeline:

- Compute a face normal from the first three vertices (right-hand rule).
- Compute face centroid.
- If your polyhedron is centered at origin, require `dot(normal, centroid) > 0` for outward. If negative, reverse vertex order.

This avoids relying on the source's winding, while still using its topology.

A consistent per-face "up direction" for text
For dice text you need, per face:
- a face normal **n** (out of the face),
- an in-face "up axis" **u** to keep digits upright/consistent.

A practical rule that works well for these kites using the face ordering above:

- Each face is listed as `{apex, a, opposite, b}` where the apex is either vertex **8** or **9** in the dataset (you can verify this from face lists). citeturn12view0  
- Define:
  - `n = normalised(cross(a - apex, b - apex))` (or based on consistent winding you enforce),
  - `u_raw = opposite - apex` (this lies in the face plane for a planar quad),
  - `u = normalised(u_raw - n * dot(n, u_raw))` to ensure it is orthogonal to the normal.

That gives each face a repeatable "top-to-bottom" axis: from the 5-valent sharp vertex ("pole") toward the far vertex on the kite. This is a strong default for orienting number decals.

Once you have `(n, u)`, you can generate UVs per face by mapping the quad into a canonical 2D kite in atlas space, or you can generate an orientation quaternion that maps face local axes to camera axes (normal toward camera, `u` toward camera-up). This aligns with the general "basis-to-basis" method used in cross-platform engines.

## Distribution in 2026: iOS (SPM vs CocoaPods) and Android (Maven)

iOS: recommended path depends on whether you require Swift-to-C++ direct calls
Apple's official distribution guidance for binary libraries is: build an **XCFramework** and distribute it as a Swift package (using SPM binary targets). citeturn14search14  

But for a C++ library specifically, there are important decision points:

- SwiftPM can enable C++ interop for Swift targets via `.interoperabilityMode(.Cxx)`, and Swift imports C++ headers via Clang modules / module maps (umbrella header pattern). citeturn13view0turn14search17  
- SwiftPM explicitly states it **does not yet support using Swift APIs in C++** (bidirectional) in SwiftPM projects, which can matter if you hoped to expose Swift back into a C++ host through generated headers during package builds. citeturn13view0  
- Shipping a *binary* XCFramework with C++ headers and expecting consumers to use Swift C++ interop has friction:
  - binary targets have no `swiftSettings`, so you can't directly put `.interoperabilityMode(.Cxx)` on the binary itself; developers have reported this as a blocker. citeturn14search2  
  - consumers often need to enable `.interoperabilityMode(.Cxx)` up the dependency chain, which has been called out as undesirable for real packages. citeturn13view0turn14search1  
  - if your imported C++ types become Swift reference types, Swift's status page indicates an iOS minimum deployment version of **16.4**, which may exceed the target range for many SDKs. citeturn1view0  

Concrete recommendation for an embeddable dice SDK in 2026
- If you want **lowest integration friction** and broad iOS support: ship an XCFramework that exposes either:
  - a **C API** (extern "C") that your Swift wrapper calls, or
  - an **Obj‑C / Obj‑C++ API** (Swift-friendly), keeping all C++/Filament types private.  
  This aligns with Filament's own iOS sample (Obj‑C++ view controller) and avoids Swift C++ interop constraints in the consumer app. citeturn8search15turn2view3turn1view0  
- If you control the app environment (internal apps) and can require iOS 16.4+ plus SwiftC++ interop mode: Swift C++ interop can work, but you should still build a narrow C++ façade that avoids unsupported template patterns and never lets exceptions escape. citeturn2view0turn2view3turn13view0  

SPM vs CocoaPods in practice for binary SDKs
SPM binary distribution is the "modern default," but Apple developer forum discussions indicate real limitations for closed-source SDKs, such as binary targets not being able to have dependencies on source-based packages in the way people expect (dependencies need to be binary as well). citeturn14search6  
This sometimes leads SDK vendors to continue offering CocoaPods as an alternative integration path when dependency graphs or resource packaging needs get complicated. (If your dice SDK embeds Filament plus other native payloads, expect to run into these edges.)

Android: the cleanest AAR-to-Maven approach, and what "Maven Central" means post-2024
Android's official docs describe AARs as the publishing unit for Android libraries and explicitly state that AARs contain compiled bytecode **and native libraries**, plus manifest/resources. citeturn15search5  

This is exactly what you want for a library that ships `jniLibs/<abi>/libdice3d.so` together with Kotlin/Java wrappers.

Publishing destinations and the 2026 reality
- Maven Central is now managed through the Sonatype Central Portal workflow. Sonatype's Central documentation provides a Maven plugin route using `central-publishing-maven-plugin` (example version shown: **0.9.0**). citeturn15search0  
- Sonatype's own Portal documentation also states that there is **no official Gradle plugin** for publishing via the Central Portal, but lists community plugin options and notes that Gradle support is on their roadmap; it explicitly calls out community solutions (including JReleaser) as current paths. citeturn15search6turn15search13  
- For private repositories (company Nexus/Artifactory), Gradle's standard `maven-publish` plugin is the straightest path; Gradle's documentation covers `maven-publish` and publication configuration. citeturn15search9  
- For private repos hosted on Sonatype Nexus Repository Manager, Sonatype provides general Maven repository documentation, and the operational pattern is "publish with maven-publish to the repo URL with credentials." citeturn15search10turn15search9  

A practical Android publishing pattern for an AAR with .so files
- Build: the Android Gradle Plugin produces the `release.aar` that already includes `jniLibs` when placed under `src/main/jniLibs/<abi>/`. This is consistent with Android's statement that AARs include native libraries. citeturn15search5  
- Publish to a private repo: use `maven-publish` and point at Nexus/Artifactory; this is the lowest-friction "ship it to teams" option. citeturn15search9turn15search10  
- Publish to Maven Central: expect to either (a) integrate an accepted community Gradle solution that targets the Central Portal, or (b) perform a Maven-based publish step using Sonatype's recommended Maven plugin approach (especially in CI), because Sonatype does not provide an official Gradle Portal plugin as of their current docs. citeturn15search6turn15search0
