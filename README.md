# Dice

A cross-platform C++ library that renders interactive 3D polyhedral dice — the
D&D set, d4 through d32 — embeddable in iOS and Android apps through native
Swift and Kotlin wrappers.

It exists because nothing quite like it did: 3D dice for mobile were either
web-only, tied to a whole game engine, or did not look good enough to put in
front of a player. This is a small, focused core that looks right out of the
box.

Rendering is [Google Filament](https://github.com/google/filament); the physics
is a lightweight tumble-and-settle simulation. **The caller decides the
outcome** — the library's job is to land the die on the face you asked for and
make the landing look honest.

## Status

Working on iOS (Metal) and Android (Vulkan). See [docs/brief.md](docs/brief.md)
for the design, the per-die geometry notes, and the known rough edges (the d32
face mapping is the interesting one).

## Building

The Filament SDK is **not** vendored — it is fetched at build time. See
[`scripts/`](scripts/) for the per-platform build scripts, and
[docs/brief.md](docs/brief.md) for the layout each platform expects.

```sh
scripts/build_ios.sh        # device + simulator static libs
scripts/build_android.sh    # arm64 .so via the NDK
```

A demo app for each platform lives in [`demo/`](demo/).

## Licence

Apache License 2.0 — see [LICENSE](LICENSE). Third-party attributions,
including Filament's, are in [NOTICE](NOTICE).
