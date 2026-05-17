# Waterfox Musl, Clang 22, Wayland-Only Build Plan

## Goal

Produce a prebuilt Waterfox artifact for Laputa that runs on arm64 Linux with
musl, uses Wayland only, avoids X11 entirely, and is built with clang/mold rather
than gcc/g++. The first useful artifact is intentionally narrow: graphical
startup of `about:blank` in a minimal Wayland kiosk environment, with no audio
stack and no desktop session manager.

The target Waterfox checkout is currently:

- Repository: `/Users/josh/d/waterfox-musl`
- Commit: `0cad8fc8ab`
- Waterfox version: `140.11.0`
- Waterfox display version: `140.11.0esr`
- Host Docker server: `linux/arm64`
- Base container: `alpine:latest`
- Alpine channel for toolchain and dependency versions: edge

The external runtime contract is from
`/Users/josh/d/kominka/xsh/laputa/WATERFOX.md`.

## Hard Requirements

- Build inside Docker from `alpine:latest`.
- Use Alpine edge package versions in general.
- Use `clang22`, `clang++-22`, LLVM 22 tools, and `mold`.
- Do not select `gcc` or `g++` for compiling or linking Waterfox or custom
  runtime libraries.
- Tolerate Alpine installing GCC packages only as dormant transitive package
  dependencies of clang/rust if edge requires them.
- Target `aarch64-unknown-linux-musl`.
- Build and package for Linux arm64, not macOS.
- Build Waterfox against a Wayland-only GTK3 stack.
- No X11, XCB, Xwayland, GLX, or X client compatibility libraries.
- No ALSA, PulseAudio, PipeWire, DBus, portals, libnotify, libsecret, speechd,
  or cups in the stage 1 browser runtime.
- Stage 1 has no audio. PipeWire audio is a later profile and artifact.
- Compile mimalloc v3 into the browser allocator path statically.
- Use fast mimalloc settings with secure mode off.
- Use Rust debug/dev builds for iteration; do not use Rust release builds until
  the final packaging profile.
- Dynamically fetched sources and build artifacts must live under ignored,
  host-mounted cache directories so Docker iteration on macOS does not refetch
  or rebuild needlessly.
- Never depend on Alpine's stock `gtk+3.0` package for the final runtime,
  because it links X libraries.
- Never depend on Alpine's stock `cage` package for the final graphical proof,
  because it depends on stock `wlroots0.20`, which pulls XCB and Vulkan.

## Latest-Version Policy

The project should treat Alpine edge as the source of truth for versions where
Alpine carries the package. Version resolution is intentionally explicit:

1. The Docker image uses edge repositories every build.
2. The wrapper has a `versions` command that reports the current edge package
   versions.
3. Source lock generation later pins source tarball URLs and SHA256s matching
   the edge versions reported at that time.
4. Upstream latest is used only when Alpine edge does not provide the required
   major version. The current example is mimalloc v3; Alpine edge currently
   provides mimalloc v2 packages only.
5. Every final artifact manifest records the Alpine release, resolved APK
   versions, source URLs, SHA256s, Waterfox commit, mozconfig, and scan results.

Edge versions observed on May 17, 2026:

- clang22 `22.1.3-r0`
- lld22 `22.1.3-r0`
- llvm22 `22.1.3-r0`
- rust `1.95.0-r0`
- cargo `1.95.0-r0`
- cbindgen `0.29.1-r0`
- sccache `0.15.0-r0`
- meson `1.11.1-r0`
- cmake `4.2.3-r0`
- ninja-build `1.13.2-r1`
- mold `2.39.1-r0`
- samurai `1.2-r8` as Meson's dormant transitive dependency
- pkgconf `2.5.1-r0`
- python3 `3.14.3-r0`
- nodejs-current `25.9.0-r0`
- gtk+3.0 `3.24.52-r0`
- cairo `1.18.4-r1`
- glib `2.88.1-r0`
- pango `1.57.1-r0`
- gdk-pixbuf `2.44.6-r0`
- harfbuzz `13.2.1-r0`
- fontconfig `2.17.1-r1`
- freetype `2.14.3-r0`
- libpng `1.6.58-r1`
- libjpeg-turbo `3.1.3-r0`
- pixman `0.46.4-r0`
- wayland `1.25.0-r0`
- wayland-protocols `1.48-r0`
- libxkbcommon `1.13.1-r0`
- libepoxy `1.5.10-r1`
- mesa `26.1.0-r0`
- cage `0.3.0-r0`
- wlroots0.20 `0.20.0-r0`
- qemu-system-aarch64 `11.0.0-r0`
- qemu-img `11.0.0-r0`
- mimalloc2 `2.2.7-r0`
- upstream mimalloc v3 latest observed from GitHub: `v3.3.2`

These observed versions are planning data, not a permanent ceiling. Before a
final source lock is generated, run `docker/waterfox-musl/wfx-musl versions`
and update the source pins to the then-current edge versions.

## Cache And Artifact Layout

All generated content for this effort lives under `.wfx-cache/` unless it is a
tracked source file:

- `.wfx-cache/apk`: APK package cache for Docker image and container work.
- `.wfx-cache/sources`: downloaded upstream source archives.
- `.wfx-cache/build`: unpacked and intermediate dependency builds.
- `.wfx-cache/sysroot`: installed custom Wayland/GTK sysroot.
- `.wfx-cache/mozbuild`: Gecko build state.
- `.wfx-cache/sccache`: C/C++ and Rust compiler cache.
- `.wfx-cache/cargo`: Cargo registry/git cache.
- `.wfx-cache/obj-aarch64-unknown-linux-musl`: Waterfox object directory.
- `.wfx-cache/dist`: packaged Waterfox artifacts and manifests.
- `.wfx-cache/test-rootfs`: assembled smoke-test rootfs content.
- `.wfx-cache/qemu`: QEMU disk images, kernel/initramfs if needed, logs, and
  screenshots.

All of these paths are gitignored and mounted into Docker containers. The
Docker context for the toolchain image is `docker/waterfox-musl/`, not the full
Waterfox tree, so image iteration does not upload the browser checkout.

## Tranches

### Current Implementation Status

- Tranche 1 is implemented: Docker image, wrapper, cache layout, and stage 1
  mozconfig exist.
- Tranche 2 is implemented far enough for the stage 1 configure/build loop:
  `docker/waterfox-musl/wfx-musl sysroot` builds the source-locked sysroot
  libraries used by configure into `.wfx-cache/sysroot`.
- The current compiled sysroot set covers zlib, expat, libffi, pcre2, libpng,
  libjpeg-turbo, mimalloc v3, freetype, fontconfig, fribidi, harfbuzz, pixman,
  cairo, glib, pango, shared-mime-info, gdk-pixbuf, atk/at-spi compatibility,
  wayland, wayland-protocols, xkeyboard-config, libxkbcommon, libepoxy, Mesa,
  and GTK 3.
- `docker/waterfox-musl/wfx-musl check` scans the generated sysroot for rejected
  libraries.
- `docker/waterfox-musl/wfx-musl sysroot-smoke` compiles and runs a small
  program against the generated headers, pkg-config files, and shared
  libraries.
- Tranche 3 is implemented and verified: the stage 1 mozconfig has explicit
  no-audio, WebMIDI/midir disable, static mimalloc replacement, Wayland-only
  GTK, clang22, and mold selections; `docker/waterfox-musl/wfx-musl configure`
  passes in Docker and then runs `docker/waterfox-musl/check-config`.
- The adapted Alpine musl patch set now covers fortify/system wrapper cleanup,
  no `execinfo.h` wrapper, stat64/large-file compatibility, the sandbox
  `sched_setscheduler` allowances, and the malloc/musl declaration fixes needed
  by this stage.
- `docker/waterfox-musl/check-config` rejects generated configs that enable X11,
  DBus, WebRTC, ALSA, PulseAudio, PipeWire, speechd, WebMIDI/midir, or non-clang
  compiler selection.
- Stage 1 now intentionally omits the built-in Rust client-certificate modules
  to get past the early PKCS#11 binding blocker quickly. The affected modules
  are `ipcclientcerts`, `osclientcerts`, and the `rsclientcerts` Rust test
  target. Normal NSS certificate verification and `trust-anchors` are still in
  the browser Rust graph.

### Current Resume Point

Last full build command used:

```sh
WFX_JOBS=2 WFX_CARGO_JOBS=2 docker/waterfox-musl/wfx-musl build
```

The last completed container build reached real Rust compilation, passed the
stage 1 client-certificate dependency cut, and failed in `style`:

- `style` failed because bindgen generated several Gecko C++ structs as
  fieldless one-byte placeholders. The main missing structs were
  `ServoComputedData`, `ComputedStyle`, `ServoElementSnapshot`,
  `AnimatedPropertyID`, `StyleSheet`, `nsStaticAtom`, `nsDynamicAtom`,
  `nsAttrName`, and `GeckoFontMetrics`.

The Cargo lockfile has been updated only for the removed `gkrust-shared`
dependencies from the stage 1 client-certificate relaxation. Files changed for
that relaxation:

- `toolkit/library/rust/shared/Cargo.toml` no longer depends on
  `ipcclientcerts` or target-gated `osclientcerts`.
- `toolkit/library/rust/shared/lib.rs` no longer imports those crates.
- `toolkit/library/rust/moz.build` no longer includes `rsclientcerts` in
  `RUST_TESTS`.
- `security/certverifier/NSSCertDBTrustDomain.cpp` makes
  `LoadIPCClientCertsModule()` return `false` without referencing the removed
  Rust symbol.

Next resume actions:

1. Continue with the `style` binding blocker. The likely fastest path is to
   teach
   `servo/components/style/build_gecko.rs` to inject raw lines into bindgen's
   `root` and `root::mozilla` modules, then manually define the fieldful Rust
   layouts for the small set of Gecko structs that clang22 bindgen emits as
   fieldless.
2. Keep using Rust debug/dev builds for iteration. Do not run release builds
   until the final packaging profile is reached.

### Tranche 1: Scaffolding And Toolchain Image

Deliverables:

- Add this `PLAN.md`.
- Add `.gitignore` entries for `.wfx-cache/`.
- Add `docker/waterfox-musl/Dockerfile`.
- Add `docker/waterfox-musl/wfx-musl`.
- Add `docker/waterfox-musl/mozconfig.stage1`.
- Build the initial Docker image.
- Verify the image reports clang22, lld22, rust, cargo, cbindgen, and sccache.

This tranche does not patch Gecko or build the custom sysroot yet.

### Tranche 2: Source Lock And Custom Wayland Sysroot

Deliverables:

- Add a source lock generator that resolves Alpine edge package versions and
  writes pinned source URLs and SHA256s.
- Build `/opt/wfx/sysroot` from source in dependency order.
- Verify every sysroot library with `llvm-readelf` and `llvm-nm`.
- Reject X11/XCB/GLX, PipeWire/Pulse/ALSA, DBus, portal, notification, secret,
  speech, and cups dependencies.

This tranche proves the GTK/Wayland runtime closure before any Waterfox build.

### Tranche 3: Musl, No-Audio, And Mimalloc Gecko Patches

Deliverables:

- Adapt the relevant Alpine Firefox musl patches.
- Add explicit no-audio configure support.
- Add Linux WebMIDI/midir disable support so ALSA is not forced.
- Add static mimalloc v3 replace-malloc integration.
- Add configure-time assertions for no X11, no audio libraries, no DBus, and
  clang-only tool selection.

This tranche gets `./mach configure` passing inside Docker.

### Tranche 4: Waterfox Build And Package

Deliverables:

- Build Waterfox using the custom sysroot and stage 1 mozconfig.
- Package `/opt/waterfox`.
- Add `/usr/bin/waterfox` wrapper content for Laputa packaging.
- Generate the artifact manifest.
- Run static dependency checks over every executable and shared object.
- Verify `waterfox --version` in a minimal rootfs without a compositor.

This tranche produces the first stage 1 artifact tarball.

### Tranche 5: Docker Headless Wayland Smoke Test

Deliverables:

- Build a separate custom test compositor stack from source.
- Use wlroots headless backend and pixman renderer.
- Build cage from source against that custom wlroots.
- Run Waterfox under cage in Docker long enough to create a Wayland surface.
- Verify no rejected libraries are loaded at runtime.

This tranche proves browser/GTK/Wayland startup in Docker. It is not a visible
DRM/KMS kiosk proof on macOS Docker.

### Tranche 6: QEMU Visible Kiosk Proof

Deliverables:

- Assemble a minimal Alpine aarch64 rootfs.
- Add custom runtime libraries, Waterfox package, custom wlroots/cage DRM
  stack, seatd/libseat, libinput, libdrm, and Mesa EGL/GLES.
- Boot with `qemu-system-aarch64`.
- Launch `cage -- /opt/waterfox/waterfox about:blank`.
- Capture a screenshot or VNC proof of the visible browser window.

This tranche is the first real graphical kiosk proof.

## Docker Toolchain Image

The toolchain image starts from `alpine:latest` and replaces repositories with:

- `https://dl-cdn.alpinelinux.org/alpine/edge/main`
- `https://dl-cdn.alpinelinux.org/alpine/edge/community`
- `https://dl-cdn.alpinelinux.org/alpine/edge/testing`

The image installs the minimum practical build tools for early iteration:

- clang22, clang22-dev, lld22, llvm22, llvm22-dev, compiler-rt, libc++-dev, mold
- rust, cargo, cbindgen
- sccache
- python3
- nodejs-current
- meson, ninja-build, cmake, pkgconf, make
- autoconf, automake, libtool, m4
- bash, coreutils, findutils, file
- curl, git, patch, rsync
- tar, xz, zstd, zip, unzip
- nasm, yasm
- linux-headers
- pax-utils

It intentionally avoids `build-base` and does not install gcc directly.
Alpine edge currently pulls GCC-related packages transitively through clang and
rust; those packages are tolerated only if `CC`, `CXX`, configure output, and
compile commands prove they are unused.

Alpine edge no longer packages `autoconf2.13` under that name. If this Waterfox
checkout requires Autoconf 2.13 during configure rather than using checked-in
configure output, add a small cached source build of Autoconf 2.13 in the
sysroot/tooling tranche instead of broadening the APK dependency set.

Required environment in the image and wrapper:

```sh
CC=clang-22
CXX=clang++-22
HOST_CC=clang-22
HOST_CXX=clang++-22
PATH=/usr/lib/ninja-build/bin:/usr/lib/llvm22/bin:$PATH
LD=mold
AR=/usr/lib/llvm22/bin/llvm-ar
RANLIB=/usr/lib/llvm22/bin/llvm-ranlib
NM=/usr/lib/llvm22/bin/llvm-nm
STRIP=/usr/lib/llvm22/bin/llvm-strip
READELF=/usr/lib/llvm22/bin/llvm-readelf
OBJCOPY=/usr/lib/llvm22/bin/llvm-objcopy
LLVM_CONFIG=llvm-config-22
RUSTC_WRAPPER=sccache
MOZBUILD_STATE_PATH=/cache/mozbuild
SCCACHE_DIR=/cache/sccache
CARGO_HOME=/cache/cargo
```

## Custom Sysroot

Install prefix:

```sh
/opt/wfx/sysroot
```

Build cache:

```sh
/cache/build
```

Source cache:

```sh
/cache/sources
```

Use `PKG_CONFIG_LIBDIR` to point only at the custom sysroot:

```sh
PKG_CONFIG_LIBDIR=/opt/wfx/sysroot/lib/pkgconfig:/opt/wfx/sysroot/share/pkgconfig
PKG_CONFIG_SYSROOT_DIR=
```

Do not allow `/usr/lib/pkgconfig` or `/usr/share/pkgconfig` to satisfy runtime
library checks for Waterfox. Build tools from `/usr` are fine; runtime
libraries that will be linked into Waterfox must come from the custom sysroot
or from bundled Gecko sources.

### Sysroot Source Set

Build source versions matching Alpine edge package versions when possible:

- expat
- libffi
- pcre2
- zlib
- libpng
- libjpeg-turbo
- freetype
- fontconfig
- fribidi
- harfbuzz
- pixman
- cairo
- glib
- pango
- shared-mime-info
- gdk-pixbuf
- at-spi2-core only if GTK cannot be built without the ATK compatibility
  libraries; otherwise prefer the smallest ATK-compatible subset.
- wayland
- wayland-protocols
- xkeyboard-config
- libxkbcommon
- libepoxy
- Mesa EGL/GLES client libraries
- GTK 3.24.x

Do not add:

- X11 libraries
- XCB libraries
- GLX support
- ALSA
- PulseAudio
- PipeWire
- DBus
- portals
- libnotify
- libsecret
- speech-dispatcher
- cups
- broadway
- GObject introspection
- docs, examples, installed tests, demos, translations unless required for a
  runtime file that GTK refuses to start without

### Important Configure Choices

GTK3:

```sh
meson setup build \
  --prefix=/opt/wfx/sysroot \
  -Dx11_backend=false \
  -Dwayland_backend=true \
  -Dbroadway_backend=false \
  -Dprint_backends=file \
  -Dcolord=no \
  -Dcloudproviders=false \
  -Dintrospection=false \
  -Ddemos=false \
  -Dexamples=false \
  -Dtests=false \
  -Dinstalled_tests=false \
  -Dgtk_doc=false \
  -Dman=false \
  -Dprofiler=false \
  -Dtracker3=false
```

libxkbcommon:

```sh
meson setup build \
  --prefix=/opt/wfx/sysroot \
  -Denable-x11=false \
  -Denable-wayland=true \
  -Denable-docs=false \
  -Denable-tools=false \
  -Denable-xkbregistry=false
```

libepoxy:

- Keep EGL/GLES dispatch.
- Disable GLX/X11 support if the version's build options expose it.
- If libepoxy cannot fully compile without X headers, patch the build to avoid
  installing or linking X/GLX paths rather than adding X runtime libraries.

Mesa:

- Enable EGL/GLES client support.
- Enable surfaceless and Wayland platforms as needed.
- Prefer llvmpipe/software rendering for the first proof.
- Disable GLX, X11 platform, Vulkan, VA-API, OpenCL, tests, demos, and docs.

Cairo:

- Enable image, png, freetype, fontconfig, and glib support.
- Disable xlib, xcb, GL, GLES if not required by GTK's cairo path, script
  surface tools, tests, and docs.

## Waterfox Configuration

The stage 1 mozconfig lives at `docker/waterfox-musl/mozconfig.stage1`.

Required options:

```sh
ac_add_options --target=aarch64-unknown-linux-musl
ac_add_options --enable-application=browser
ac_add_options --enable-default-toolkit=cairo-gtk3-wayland-only
ac_add_options --enable-linker=mold
ac_add_options --with-ccache=sccache
ac_add_options --with-app-basename=Waterfox
ac_add_options --with-app-name=waterfox
ac_add_options --with-branding=waterfox/browser/branding
ac_add_options --with-distribution-id=net.waterfox
```

Fast/minimal stage 1 options:

```sh
ac_add_options --disable-crashreporter
ac_add_options --disable-debug
ac_add_options --disable-debug-symbols
ac_add_options --disable-dmd
ac_add_options --disable-geckodriver
ac_add_options --disable-profiling
ac_add_options --disable-tests
ac_add_options --disable-updater
ac_add_options --disable-dbus
ac_add_options --disable-necko-wifi
ac_add_options --disable-webrtc
ac_add_options --disable-printing
ac_add_options --disable-synth-speechd
ac_add_options --disable-webspeech
ac_add_options --disable-ffmpeg
ac_add_options --disable-av1
ac_add_options --disable-jxl
ac_add_options --without-wasm-sandboxed-libraries
ac_add_options --enable-optimize="-O1 -g0 -w"
```

New options to add in tranche 3:

```sh
ac_add_options --enable-audio-backends=none
ac_add_options --disable-webmidi-midir
ac_add_options --enable-mimalloc-replace
ac_add_options --with-mimalloc-prefix=/opt/wfx/sysroot
```

Do not enable Alpine system NSS, NSPR, SQLite, ICU, media codec, or other
browser-private dependencies. Use Gecko bundled copies for those unless a
specific build failure proves otherwise.

## Musl Patch Plan

Use `/Users/josh/d/kominka/aports/community/firefox/` as reference material,
not as a blind patch queue.

Patches expected to matter for this aarch64 musl stage:

- fortify/system-wrapper cleanup
- `sched_setscheduler` sandbox fix
- large-file/musl compatibility
- removal of `execinfo.h` assumptions if still present in this Waterfox tree

Patches expected to be skipped initially:

- ppc-specific patches
- riscv-specific patches
- loongarch-specific patches
- Widevine patches
- rust LTO patches
- patches already represented in this Waterfox version

Every adapted patch must be documented in the artifact manifest with the Alpine
patch name and the reason it was used or skipped.

## No-Audio Plan

The current configure surface does not cleanly express "Linux with no cubeb
backend and no WebMIDI ALSA/midir". Tranche 3 adds that surface explicitly.

Required behavior:

- `--enable-audio-backends=none` is accepted.
- No `MOZ_ALSA`.
- No `MOZ_PULSEAUDIO`.
- No PipeWire checks.
- No `asound` package-config check.
- No `OS_LIBS += ["asound"]` from WebMIDI/midir.
- Browser startup and `about:blank` do not require an audio library.

This intentionally breaks audio for the first artifact. Later PipeWire support
must be a separate mozconfig and separate artifact.

## Mimalloc v3 Plan

Alpine edge currently carries mimalloc v2 packages, not mimalloc v3. Use the
latest upstream v3 release when the mimalloc source lock is generated. The
observed latest upstream release is `v3.3.2`.

Integration approach:

- Fetch pinned mimalloc v3 source into `.wfx-cache/sources`.
- Build a static library only.
- Use fast non-secure options in the iterative build; final packaging can
  revisit allocator tuning.
- Disable secure mode.
- Install headers and the static archive into `/opt/wfx/sysroot`.
- Add `memory/replace/mimalloc/` with a replace-malloc shim.
- Add `MOZ_MIMALLOC_REPLACE`.
- Link the mimalloc replace implementation statically.
- Keep Gecko's allocator bridge intact.
- Ensure the final Waterfox artifact does not need `libmimalloc.so`.

Acceptance:

- `MOZ_MIMALLOC_REPLACE=1` appears in configure output.
- `llvm-readelf -d` finds no `libmimalloc.so` dependency.
- The replace-malloc object is present in the linked binary or libxul path.
- Basic startup works with mimalloc enabled.

## Packaging Plan

Package layout:

```text
/opt/waterfox/
/opt/waterfox/waterfox
/opt/waterfox/waterfox-bin
/opt/waterfox/libxul.so
/usr/bin/waterfox
```

Wrapper environment:

```sh
MOZ_ENABLE_WAYLAND=1
GDK_BACKEND=wayland
NO_AT_BRIDGE=1
MOZ_DISABLE_AUTO_SAFE_MODE=1
MOZ_CRASHREPORTER_DISABLE=1
```

The wrapper must not set `DISPLAY`.

The package must not include a desktop file, icon cache update, MIME database
update, portal integration, notification integration, or DBus activation file
in stage 1.

Updates are owned by the package manager:

- Build updater support off.
- Add default preferences only for non-interactive update/crash/first-run
  policy.
- Do not encode user profile data in the package.

## Static Acceptance Checks

Run checks from inside the Alpine container with LLVM tools from the same image.

Reject these `DT_NEEDED` entries in every executable and shared object under
`/opt/waterfox` and the stage 1 custom runtime closure:

```text
libX11
libX11-xcb
libxcb
libxcb-shm
libxcb-composite
libxcb-dri3
libxcb-ewmh
libxcb-icccm
libxcb-present
libxcb-render
libxcb-render-util
libxcb-res
libxcb-xfixes
libxcb-xinput
libXcomposite
libXdamage
libXext
libXfixes
libXi
libXinerama
libXrandr
libXrender
libXcursor
libxkbfile
libXt
libSM
libICE
libGLX
libasound
libpipewire
libpulse
libdbus
libnotify
libsecret
libspeechd
libcups
libmimalloc.so
```

Reject these dynamic symbols:

```text
gdk_x11_
XOpenDisplay
XCloseDisplay
xcb_connect
xcb_disconnect
glX
```

Configure assertions:

- `MOZ_WAYLAND=1`
- `MOZ_X11` absent
- `MOZ_ENABLE_DBUS` absent
- `NECKO_WIFI` absent
- `MOZ_WEBRTC` absent
- `MOZ_ALSA` absent
- `MOZ_PULSEAUDIO` absent
- `MOZ_SYSTEM_PIPEWIRE` absent
- `MOZ_SYNTH_SPEECHD` absent
- `MOZ_MIMALLOC_REPLACE=1`

Toolchain assertions:

- Configure chooses `clang-22`.
- Configure chooses `clang++-22`.
- Linker is `mold`.
- Compile commands contain no selected `gcc` or `g++`.
- `gcc` may exist in the image only as a dormant transitive dependency.

## Docker Headless Graphical Smoke Test

Docker Desktop on macOS cannot provide a real Linux DRM/KMS device. The Docker
smoke test therefore proves Wayland client startup and compositor protocol
startup, not visible hardware graphics.

Do not install Alpine's stock `cage` or `wlroots0.20` packages for this proof.
Their edge dependency closure includes XCB and Vulkan libraries. Build a
separate compositor test stack from source.

wlroots headless test build:

```sh
meson setup build \
  --prefix=/opt/wfx/test-compositor \
  -Dauto_features=disabled \
  -Dbackends=[] \
  -Drenderers=[] \
  -Dallocators=[] \
  -Dxwayland=disabled \
  -Dsession=disabled \
  -Dexamples=false \
  -Dtests=false \
  -Dcolor-management=disabled \
  -Dlibliftoff=disabled \
  -Dxcb-errors=disabled
```

The wlroots headless and pixman paths are always built by wlroots and avoid
DRM/input/session/Xwayland for this test profile.

cage test build:

```sh
meson setup build \
  --prefix=/opt/wfx/test-compositor \
  -Dman-pages=disabled
```

Smoke command:

```sh
XDG_RUNTIME_DIR=/run/user/0 \
WAYLAND_DISPLAY=wayland-0 \
WLR_BACKENDS=headless \
WLR_RENDERER=pixman \
WLR_HEADLESS_OUTPUTS=1 \
MOZ_ENABLE_WAYLAND=1 \
GDK_BACKEND=wayland \
NO_AT_BRIDGE=1 \
MOZ_WEBRENDER_SOFTWARE=1 \
LIBGL_ALWAYS_SOFTWARE=1 \
cage -- /opt/waterfox/waterfox about:blank
```

The wrapper should:

- Create `/run/user/0` with mode `0700`.
- Start cage.
- Allow enough time for Waterfox to create a Wayland toplevel.
- Capture stdout/stderr logs.
- Terminate cleanly after the startup window exists or after a timeout.
- Fail if logs show GTK cannot initialize Wayland.
- Fail if logs show attempts to load any rejected library.
- Fail if any process exits from a missing library or missing symbol.

This proof is fast and should run often.

## QEMU Visible Kiosk Proof

The first visible graphical proof should use QEMU rather than Docker on macOS.

QEMU target:

- `qemu-system-aarch64`
- HVF acceleration on macOS when available
- virtio block device
- virtio GPU
- virtio keyboard
- virtio tablet or mouse
- serial console log
- Cocoa display for manual runs
- Optional VNC or screenshot mode for automated proof

Rootfs contents:

- Alpine arm64 userspace
- packaged Waterfox artifact under `/opt/waterfox`
- `/usr/bin/waterfox` wrapper
- custom stage 1 runtime libraries
- custom wlroots/cage DRM stack
- libdrm
- Mesa EGL/GLES
- libinput
- libseat and seatd
- libevdev
- mtdev
- eudev/libudev-compatible minimal runtime if wlroots/libinput needs it
- fonts sufficient to render readable page text

Rootfs exclusions:

- Xwayland
- X11/XCB libraries
- DBus session or system daemon
- desktop portals
- PipeWire
- PulseAudio
- ALSA for stage 1
- display manager
- session manager

Boot command inside the guest:

```sh
mkdir -p /run/user/0
chmod 0700 /run/user/0
export XDG_RUNTIME_DIR=/run/user/0
export WAYLAND_DISPLAY=wayland-0
export MOZ_ENABLE_WAYLAND=1
export GDK_BACKEND=wayland
export NO_AT_BRIDGE=1
export MOZ_WEBRENDER_SOFTWARE=1
export LIBGL_ALWAYS_SOFTWARE=1
seatd -g root &
cage -- /opt/waterfox/waterfox about:blank
```

QEMU acceptance:

- Guest boots to the kiosk command.
- `seatd` creates a usable seat socket.
- cage starts with DRM backend and no Xwayland.
- Waterfox opens `about:blank`.
- Keyboard and pointer input reach the browser.
- Logs contain no load attempts for rejected libraries.
- A screenshot or VNC session shows the browser window.
- Shutdown returns cleanly.

## Final Artifact Manifest

Each packaged artifact gets a manifest next to the tarball in
`.wfx-cache/dist`.

Required fields:

- Waterfox git commit and dirty-state marker.
- Waterfox version files.
- Build date.
- Docker image ID.
- Alpine release.
- APK repository URLs.
- APK package versions.
- Source lock URLs and SHA256s.
- Full mozconfig.
- Toolchain versions.
- Compiler/linker path checks.
- Sysroot dependency scan results.
- Waterfox dependency scan results.
- Rejected-symbol scan results.
- `waterfox --version` output.
- Docker headless smoke result once implemented.
- QEMU kiosk proof result once implemented.

## References

- Laputa Waterfox contract:
  `/Users/josh/d/kominka/xsh/laputa/WATERFOX.md`
- Alpine Firefox musl patch reference:
  `/Users/josh/d/kominka/aports/community/firefox/`
- Alpine GTK packaging reference:
  `/Users/josh/d/kominka/aports/community/gtk+3.0/`
- Alpine mimalloc packaging reference:
  `/Users/josh/d/kominka/aports/community/mimalloc2/`
- mimalloc upstream:
  `https://github.com/microsoft/mimalloc`
- wlroots upstream:
  `https://gitlab.freedesktop.org/wlroots/wlroots`
- cage upstream:
  `https://github.com/cage-kiosk/cage`
