# Agent Handoff: Waterfox Musl Build

This repository is a Waterfox checkout adapted to produce arm64 Linux musl
artifacts for Laputa. The first milestone is a narrow, visible browser proof:
Waterfox starts on `about:blank` under a minimal Wayland-only kiosk environment,
built with Alpine, clang/LLVM, mold, and musl, with no X11 and no audio stack.

Alpine's Firefox package and patches are available at
`/Users/josh/d/kominka/aports/community/firefox/` and remain the first reference
when adapting musl or packaging fixes.

## Hard Requirements

- Build and package for Linux arm64 musl, not macOS.
- Use Alpine edge for build-time dependencies unless a pinned source build is
  intentionally used.
- Use clang/LLVM and mold for compiling and linking Waterfox and source-built
  runtime libraries. Do not select GCC/G++ as the compiler/linker.
- Alpine may install GCC runtime packages such as `libgcc` or `libstdc++` as
  dormant transitive runtime dependencies.
- Build-time dependencies may come from Alpine. The custom `/opt/wfx/sysroot`
  is the runtime closure used for packaging and smoke/runtime verification; it
  is not a mandatory SDK for `./mach build`.
- Build Waterfox against a Wayland-only GTK3 stack.
- Do not ship X11, XCB, Xwayland, GLX, Vulkan, audio, DBus, portal,
  notification, secret-service, speechd, or cups libraries in the stage 1
  browser runtime.
- Stage 1 has no audio. PipeWire/audio support must be a separate future
  profile and artifact.
- Mimalloc v3 is linked through the browser allocator path statically. The final
  Waterfox artifact must not need `libmimalloc.so`.
- Debug/dev builds are the default iteration path. Release builds use the
  separate `mozconfig.release` and release objdir only when explicitly requested.
- Do not run pre-commit hooks. Do not push.

## Status

- Docker toolchain image, wrapper, cache layout, and stage 1 mozconfigs are in
  place.
- The custom Wayland/GTK runtime sysroot builds and is used for packaged
  runtime checks.
- Gecko configure work is in place: Wayland-only GTK, no audio, WebMIDI/midir
  disabled, clang/mold selected, mimalloc v3 static replacement, musl patches,
  and configure checks.
- Debug Waterfox build and package pass.
- Docker headless Wayland smoke passes and verifies an `xdg_toplevel`.
- QEMU boots to seatd + custom cage/wlroots + Waterfox. Cocoa QEMU visibly
  renders Waterfox, repeat boots no longer hit profile read-only panics, host
  terminal `Ctrl-C` terminates QEMU cleanly, keyboard/mouse input work, and QEMU
  user networking works.
- WaterfoxBlocker is excluded from the musl build with `--disable-waterfox-blocker`;
  staged artifact scans find no blocker files or registration strings.
- The old URL bar/menu rendering failure is fixed. Root cause was zero-sized
  chrome fonts from invalid GTK look-and-feel text scale; the durable fix is in
  `widget/gtk/nsLookAndFeel.cpp`.
- Release build and package pass in the separate release objdir, producing a
  staged release artifact and manifest with dependency scans.
- The experimental folded-library release builds and packages, but it is not a
  usable release path yet: QEMU boots and browser chrome works, while loading
  normal websites fails with the in-content "Try Again" network error page.
  Debug and normal release QEMU images can load `google.com`, so the regression
  is specific to the folded release profile.

## Main Commands

Run commands from the repository root.

Build or refresh the toolchain image:

```sh
docker/waterfox-musl/wfx-musl image
```

Report current Alpine edge versions:

```sh
docker/waterfox-musl/wfx-musl versions
```

Configure and validate debug config:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl configure
```

Build debug Waterfox:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl build
```

Package debug Waterfox and run static dependency checks:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl package
```

Rerun the staged Waterfox ELF dependency check without repackaging:

```sh
docker/waterfox-musl/wfx-musl waterfox-deps
```

Headless Wayland smoke:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl smoke-headless
```

Build custom DRM/libinput kiosk compositor:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl kiosk-compositor
```

Build QEMU image:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl qemu-image
```

Run visible QEMU proof:

```sh
WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 docker/waterfox-musl/wfx-musl qemu-run
```

Serial-only QEMU proof:

```sh
timeout 90s env WFX_QEMU_DISPLAY=none WFX_QEMU_ACCEL=hvf docker/waterfox-musl/wfx-musl qemu-run
```

Release path, only when explicitly requested:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl configure-release
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl build-release
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl package-release
```

If a fresh release objdir fails early with missing
`dist/system_wrappers/.moz-case-probe`, create the directory and rerun:

```sh
mkdir -p .wfx-cache/obj-aarch64-alpine-linux-musl-release/dist/system_wrappers
```

## Development Loop

Prefer the narrowest loop that exercises the code changed.

1. Gecko source/config changes: run `configure` first if build flags changed,
   then `build`, then `package`.
2. Runtime sysroot changes: run `sysroot`, `check`, and `sysroot-smoke` before
   rebuilding Waterfox unless the change only affects QEMU packaging.
3. Headless compositor/browser runtime changes: run `smoke-headless`. It is
   faster than QEMU and catches loader, GTK, and Wayland surface regressions.
4. QEMU rootfs/init changes: run `qemu-image`, then a bounded serial-only
   `qemu-run`. Use Cocoa once serial shows seatd, wlroots, and Waterfox
   surfaces.
5. Keep `WFX_JOBS=8 WFX_CARGO_JOBS=8` while OrbStack has enough memory. If
   memory pressure returns, reduce jobs before changing code.

Expected warm-cache timings:

- Tiny incremental C++ build: seconds to minutes.
- `package`: slower than incremental build because staging and tar/xz are mostly
  serial.
- `qemu-image`: tens of seconds after APK/cache warmup.
- QEMU boot to Waterfox: about 20-30 seconds.

## Cache And Artifacts

Generated content lives under `.wfx-cache/` and is not tracked.

- `.wfx-cache/apk`: APK cache.
- `.wfx-cache/sources`: source tarballs.
- `.wfx-cache/build`: unpacked/intermediate dependency builds.
- `.wfx-cache/build-deps`: build-only source-built deps, such as mimalloc v3.
- `.wfx-cache/sysroot`: runtime sysroot mounted as `/opt/wfx/sysroot`.
- `.wfx-cache/mozbuild`: Gecko build state.
- `.wfx-cache/sccache`: compiler cache.
- `.wfx-cache/cargo`: Cargo cache.
- `.wfx-cache/obj-aarch64-alpine-linux-musl`: debug Waterfox objdir.
- `.wfx-cache/obj-aarch64-alpine-linux-musl-release`: release Waterfox objdir.
- `.wfx-cache/dist`: staged roots, packages, manifests, dependency reports,
  and smoke reports.
- `.wfx-cache/kiosk-compositor`: custom DRM/libinput cage/wlroots install.
- `.wfx-cache/qemu`: QEMU rootfs, kernel, initramfs, disk image, and repro
  screenshots/logs.

Current debug artifact path:

```text
.wfx-cache/dist/waterfox-140.11.0esr.en-US.linux-musl-aarch64.stage1-debug.tar.xz
```

Current release artifact path:

```text
.wfx-cache/dist/waterfox-140.11.0esr.en-US.linux-musl-aarch64.stage1-release.tar.xz
```

Release packaging uses `.wfx-cache/dist/release-root`. The latest release
manifest is `.wfx-cache/dist/waterfox-140.11.0esr.en-US.linux-musl-aarch64.manifest.txt`
and records `mozconfig.release`, `elf_count=22`, the needed-library report, and
the rejected-dependency report.

The squashed `/tmp` checkout may not initially have all caches. Reuse caches
from `/Users/josh/d/waterfox-musl/.wfx-cache/` only when needed. For QEMU-only
iteration, `dist/stage1-root` and `kiosk-compositor` are usually enough.

## Version And Source Policy

Alpine edge is the source of truth for package versions when Alpine carries the
needed package. Run `docker/waterfox-musl/wfx-musl versions` before refreshing
source locks or final manifests.

Use upstream latest only when Alpine edge does not provide the required major
version. The current example is mimalloc v3: Alpine edge carries mimalloc v2,
so v3 is source-built and pinned in `docker/waterfox-musl/sources.lock`.

Every final artifact manifest records the Waterfox git commit/dirty state,
Alpine release, toolchain commands, mozconfig, APK package list, source lock,
dependency reports, and artifact paths.

## Build Configuration

The debug mozconfig is `docker/waterfox-musl/mozconfig.stage1`.

Important debug choices:

- `--host=aarch64-alpine-linux-musl`
- `--target=aarch64-alpine-linux-musl`
- `--enable-default-toolkit=cairo-gtk3-wayland-only`
- `--enable-linker=mold`
- `--enable-debug`
- `--disable-optimize`
- `--enable-rust-debug`
- `--enable-audio-backends=none`
- `--disable-webmidi-midir`
- `--enable-mimalloc-replace`
- `--with-mimalloc-prefix=/opt/wfx/build-deps/mimalloc`
- `--disable-waterfox-blocker`

The release mozconfig is `docker/waterfox-musl/mozconfig.release`.

Important release choices:

- Separate objdir: `/cache/obj-aarch64-alpine-linux-musl-release`.
- `--enable-release`
- `--disable-debug`
- `--enable-optimize=-O2`
- `--disable-debug-symbols`
- `--disable-rust-debug`
- Same musl, Wayland-only, no-audio, no-X, clang/mold, static mimalloc, and
  WaterfoxBlocker exclusion constraints as debug.

Do not enable Alpine system NSS, NSPR, SQLite, ICU, media codec, or other large
system-library substitutions as part of stage 1 unless a task explicitly targets
that dependency policy.

### Folded Release TODO

The folded release profile is experimental and must not replace the normal
release artifact until website loading is fixed and covered by a QEMU smoke.
It is selected by `docker/waterfox-musl/mozconfig.folded-release` and the
`configure-folded-release`, `build-folded-release`, and
`package-folded-release` commands.

Known current state:

- Builds and packages successfully.
- Folds away separate `libmozsqlite3`, NSPR, and NSS helper libraries from the
  staged `/opt/waterfox` dependency list.
- Boots in QEMU and renders browser chrome.
- Fails to load normal websites, showing the browser "Try Again" page instead.
- The same QEMU image path built from debug `stage1-root` and normal
  `release-root` can load `google.com`.

Future investigation should first isolate which folded library causes the
network/page-load regression. Start by partially backing out folding around
NSS/NSPR and SQLite rather than changing QEMU, networking, or the kiosk
compositor. Add a QEMU smoke that opens an HTTPS URL and treats the in-content
"Try Again" page as failure before considering folded release usable.

## Runtime Sysroot

The custom runtime sysroot installs into:

```text
/opt/wfx/sysroot
```

It is assembled from source-locked runtime libraries and copied into staged
artifacts under `/opt/wfx/sysroot`. It is validated when assembling and
smoke-testing the final runtime closure. Waterfox build-time pkg-config checks
may use Alpine packages from `/usr`.

The current source-built runtime closure covers zlib, expat, libffi, pcre2,
libpng, libjpeg-turbo, freetype, fontconfig, pixman, fribidi, harfbuzz, cairo,
wayland, wayland-protocols, xkeyboard-config, libxkbcommon, gdk-pixbuf, and GTK
3. Mimalloc v3 is separate under `/opt/wfx/build-deps/mimalloc` as a build-only
static dependency.

Do not use Alpine stock `gtk+3.0` for the final runtime because it pulls X
libraries. Do not use Alpine stock `cage` for the QEMU proof because its
wlroots stack pulls unwanted XCB/Vulkan dependencies.

## Dependency And Rejection Rules

Static scans are in `check-waterfox-deps`, `package-stage1`,
`build-test-compositor`, `build-kiosk-compositor`, and `check-config`.

The final staged Waterfox scan compares every NEEDED entry against
`docker/waterfox-musl/waterfox-allowed-needed.txt` and hard-fails on rejected
dependencies. Runtime smoke also scans loaded libraries from `/proc/*/maps`.

Rejected runtime families include:

- X11, X11-xcb, XCB, Xcomposite, Xcursor, Xdamage, Xext, Xfixes, Xi, Xinerama,
  Xrandr, Xrender, Xtst, Xxf86vm, xkbfile, Xwayland, and GLX.
- Vulkan.
- ALSA, PulseAudio, PipeWire audio.
- DBus, portals, notification, secret-service.
- speechd and cups.
- dynamic `libmimalloc.so`.

Relrhack and packed relative relocations are disabled. The earlier path emitted
Android packed relocation dynamic tags; Alpine musl did not apply those during
`dlopen`, which crashed during NSPR initialization. Do not re-enable without
checking `llvm-readelf` output and a packaged runtime smoke.

## QEMU Proof

Preferred visible command:

```sh
WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 docker/waterfox-musl/wfx-musl qemu-run
```

Default path is `hvf + virtio + cocoa`. QEMU user networking is enabled by
default. The guest profile lives in `/run/wfx-profile`, so repeat boots do not
dirty the root image or hit read-only profile cleanup failures.

Important QEMU defaults and knobs:

- `WFX_QEMU_DISPLAY`, default `cocoa,zoom-to-fit=on,show-cursor=on`.
- `WFX_QEMU_ACCEL`, default `hvf`.
- `WFX_QEMU_GPU`, default `virtio` with `hvf`, `bochs` with `tcg`.
- `WFX_QEMU_WIDTH`, default `1024`; use `800` for faster visible iteration.
- `WFX_QEMU_HEIGHT`, default `768`; use `600` for faster visible iteration.
- `WFX_QEMU_INPUT`, default `virtio`; accepted values are `virtio`, `usb`,
  `both`, and `none`.
- `WFX_QEMU_NETWORK`, default `user`; `none` disables networking.
- `WFX_QEMU_SERIAL`, default `stdio`.
- `WFX_QEMU_MONITOR`, default `none`.
- `WFX_QEMU_QMP`, optional QMP socket.
- `WFX_QEMU_GPU_OPTS`, optional raw comma-separated device options.
- `WFX_QEMU_VNC`, optional VNC endpoint.
- `WFX_QEMU_GUEST_BROWSER`, accepted values are `waterfox`, `firefox`, and
  `gtk-popup-repro`.
- `WFX_QEMU_GUEST_COMPOSITOR`, accepted values are `cage` and `weston`.
- `WFX_QEMU_GUEST_WLR_RENDERER`, optional guest override for `WLR_RENDERER`.
- `WFX_QEMU_GUEST_DRM_NO_ATOMIC`, optional guest override for
  `WLR_DRM_NO_ATOMIC`.
- `WFX_QEMU_GUEST_SOFTWARE_WEBRENDER=1`, diagnostic Software WebRender/SWGL
  comparison.
- `WFX_QEMU_GUEST_FULL_DAMAGE=0`, disables the QEMU-proof default
  `MOZ_WAYLAND_FULL_DAMAGE=1`.
- `WFX_QEMU_INCLUDE_ALPINE_FIREFOX=1`, build-time image option for Alpine
  Firefox A/B diagnostics.
- `WFX_QEMU_INCLUDE_WESTON=1`, build-time image option for Weston compositor
  A/B diagnostics. Increase image size with `WFX_QEMU_IMAGE_MB=3072`.

If the Cocoa window is blank, check the launch line first. `hvf` should use
`gpu=virtio`; `tcg` should use `gpu=bochs`. `hvf + bochs + Cocoa` can leave the
host window black even when QEMU `screendump` captures a valid framebuffer, so
it is diagnostic-only and requires `WFX_QEMU_ALLOW_HVF_BOCHS=1`.

`WFX_QEMU_GPU=virtio-mmio` currently fails before browser launch in direct
kernel boot because wlroots sees zero DRM GPUs. `stdvga` and `secondary-vga`
are diagnostic display paths. `cirrus` with HVF trips QEMU's framebuffer
page-alignment assertion and is blocked.

## Automated QEMU Repros

Use `docker/waterfox-musl/qemu-repro-hamburger` as the fast visible regression
loop for menu/text rendering. It boots QEMU, waits for Waterfox, clicks through
QMP, writes full-frame PPM screenshots, writes a right-edge strip, records
luma-based black-screen stats in `repro.txt`, and exits nonzero on effectively
black frames.

Examples:

```sh
env WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 \
  WFX_REPRO_ID=hamburger WFX_REPRO_BOOT_WAIT=15 WFX_REPRO_AFTER_CLICK_WAIT=4 \
  docker/waterfox-musl/qemu-repro-hamburger
```

```sh
env WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 \
  WFX_REPRO_ID=context WFX_REPRO_BOOT_WAIT=15 WFX_REPRO_AFTER_CLICK_WAIT=4 \
  WFX_REPRO_CLICK_X=400 WFX_REPRO_CLICK_Y=300 WFX_REPRO_BUTTON=right \
  docker/waterfox-musl/qemu-repro-hamburger
```

```sh
env WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 \
  WFX_REPRO_ID=urlbar WFX_REPRO_BOOT_WAIT=15 WFX_REPRO_AFTER_CLICK_WAIT=2 \
  WFX_REPRO_CLICK_X=330 WFX_REPRO_CLICK_Y=49 WFX_REPRO_TEXT=abc.com \
  docker/waterfox-musl/qemu-repro-hamburger
```

The final passing debug repros were:

- `.wfx-cache/qemu/repro-hamburger-final-hamburger/after-hamburger.png`
- `.wfx-cache/qemu/repro-hamburger-final-context/after-hamburger.png`
- `.wfx-cache/qemu/repro-hamburger-final-urlbar/after-hamburger.png`

The GTK control app is `docker/waterfox-musl/gtk-popup-repro.c`, packaged into
the QEMU image as `/usr/bin/wfx-gtk-popup-repro`. Run it with:

```sh
WFX_QEMU_GUEST_BROWSER=gtk-popup-repro docker/waterfox-musl/qemu-repro-hamburger
```

It proved generic GTK/Wayland popups rendered correctly in the same guest before
the Gecko zero-font root cause was found.

## Important Fixes And Diagnostics

QEMU/rootfs issues already solved:

- Direct kernel boot could not see `/dev/vda`; `qemu-image` now creates
  `initramfs-virt`.
- `apk --no-scripts` left BusyBox applets missing; `qemu-image` installs
  applet symlinks with `busybox --install -s`.
- Cage needed `libgcc_s.so.1`; Waterfox needed `libstdc++.so.6`; GTK needed
  `libintl.so.8`.
- Alpine `seatd` does not support the earlier `-s` socket option.
- QEMU input requires wlroots `drm,libinput` backends; virtio keyboard/tablet
  now create guest event devices.
- Waterfox glxtest no longer reports missing `libpci` or `libEGL` after adding
  Mesa EGL/GLES, GBM, Gallium, and `pciutils-libs` to the QEMU rootfs.
- Cage debugoptimized builds used to force wlroots debug logs. The Cage patch
  now defaults runtime wlroots logging to errors while preserving `-D` as the
  explicit debug opt-in.
- The kiosk profile disables TRR/DoH and ORB JavaScript validation to keep QEMU
  networking native and avoid a debug-only JSOracle utility-process assertion.

Waterfox/Gecko issues already solved:

- `browser/locales/en-US/browser/waterfox.ftl` restores a resource Waterfox
  browser chrome loads unconditionally. Without it, URL bar typing hit
  `selectedBrowser` and `UrlbarInput` exceptions after localization bundle
  failures.
- `widget/gtk/nsLookAndFeel.cpp` clamps invalid GTK text scale from
  `gdk_screen_get_resolution()` to `1.0f` and falls back when GTK/Pango reports
  a missing family or non-positive font size. This fixed invisible URL bar text
  and blank/garbled menu popups.
- `--disable-waterfox-blocker` prevents WaterfoxBlocker from being built or
  registered in the musl build. The QEMU kiosk profile also disables blocker
  prefs as a runtime fallback.

Useful historical diagnostics:

- Alpine Firefox reproduced the same UI repaint failures as Waterfox before the
  font-scale fix, so the Waterfox feature set was not the primary cause.
- Weston reproduced the same UI repaint failures as cage/wlroots before the
  font-scale fix, so the issue was not cage-specific.
- Software WebRender/SWGL could log
  `RenderCompositorSWGL failed mapping default framebuffer, no dt` while
  clicking popups; it is no longer forced by default.
- `virtio_gpu: driver missing` and WebRender texture-crop warnings were
  investigated while the font issue was unresolved; do not chase them again
  unless a new visible rendering failure appears.

## Debugging References

Useful logs and reports:

- `.wfx-cache/build/smoke-headless/cage-waterfox.log`
- `.wfx-cache/build/smoke-headless/loaded-libraries.txt`
- `.wfx-cache/dist/*.manifest.txt`
- `.wfx-cache/dist/*.headless-smoke.txt`
- `.wfx-cache/dist/*deps.txt`
- `.wfx-cache/qemu/repro-hamburger-*/repro.txt`
- QEMU serial output from `qemu-run`

For dependency resolution inside the assembled QEMU rootfs:

```sh
docker run --rm --platform linux/arm64 \
  --volume "$PWD/.wfx-cache:/cache" \
  waterfox-musl:edge-clang22 \
  chroot /cache/qemu/rootfs /bin/sh -euc \
  'export LD_LIBRARY_PATH=/opt/wfx/kiosk-compositor/lib:/opt/waterfox:/opt/wfx/sysroot/lib; ldd /opt/waterfox/waterfox; ldd /opt/wfx/kiosk-compositor/bin/cage'
```

## Completion Criteria

Before calling a build profile complete, verify the relevant subset:

- configure/check-config passes
- build passes
- package and static dependency scans pass
- headless Wayland smoke passes for debug/runtime changes
- WaterfoxBlocker remains excluded
- QEMU image is rebuilt from the current staged root
- QEMU boots twice in a row without profile/rootfs panics
- Cocoa QEMU visibly renders Waterfox
- `Ctrl-C` from a real terminal terminates QEMU cleanly
- keyboard and mouse input reach the guest
- hamburger menu, context menu, and URL bar typed text pass the QEMU repro
  screenshots
