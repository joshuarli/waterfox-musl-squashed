# Agent Handoff: Waterfox Musl Build

This repository is a Waterfox checkout being adapted to produce an arm64 Linux
musl artifact for Laputa. The target is narrow: a debug/dev Waterfox build that
runs `about:blank` under a minimal Wayland-only kiosk environment, with clang,
LLVM, mold, musl, no X11, and no audio stack.

Read `PLAN.md` for the full tranche plan and historical decisions. This file is
the practical development loop for coding agents so they can continue without
rediscovering the same build, runtime, and QEMU issues.

## Current Working Assumptions

- Active worktree for this squashed handoff: `/tmp/waterfox-musl-squashed`.
- Original unsquashed working tree used during bring-up:
  `/Users/josh/d/waterfox-musl`.
- Alpine Firefox packaging and patch reference:
  `/Users/josh/d/kominka/aports/community/firefox/`.
- Build-time dependencies may come from Alpine. The custom `/opt/wfx/sysroot`
  is the runtime closure for packaging and smoke/runtime verification, not a
  mandatory SDK for `./mach build`.
- Keep iteration on debug/dev builds. Do not build release binaries.
- Do not run pre-commit hooks. Do not push.
- Do not use GCC/G++ as the selected compiler/linker. Alpine may install GCC
  runtime packages such as `libgcc` or `libstdc++` when they are required at
  runtime.

## Tranche Status

- Tranche 1: Docker toolchain image, wrapper, and cache layout are in place.
- Tranche 2: custom Wayland/GTK runtime sysroot builds and is used for packaged
  runtime checks.
- Tranche 3: Gecko configure work is in place: Wayland-only GTK, no audio,
  WebMIDI/midir disabled, clang/mold selected, mimalloc v3 static replacement,
  musl patches, and configure checks.
- Tranche 4: debug Waterfox build and package have passed.
- Tranche 5: Docker headless Wayland smoke passes and verifies an
  `xdg_toplevel`.
- Tranche 6: QEMU boots to seatd + custom cage/wlroots + Waterfox surfaces.
  Cocoa QEMU visibly renders Waterfox, repeat boots no longer hit profile
  read-only panics, and host terminal `Ctrl-C` terminates QEMU cleanly.
  Keyboard and mouse input now defaults to virtio keyboard/tablet devices with
  USB HID available as a comparison mode. Mouse and keyboard input have been
  manually verified in Cocoa. QEMU user networking is enabled by default and
  serial boot verifies a DHCP lease. The remaining manual Cocoa check is
  whether the URL bar text repaint issue is fixed after adding Mesa EGL/GLES
  runtime libraries.
- After the WaterfoxBlocker build exclusion, the stage 1 debug build and package
  passed, and artifact scans found no blocker files or registration strings in
  the staged root.

Current important local edits in this squashed tree:

- `docker/waterfox-musl/qemu-image`
  - creates an initramfs for virtio root mounting
  - installs BusyBox applet symlinks after `apk --no-scripts`
  - includes `libgcc`, `libstdc++`, `libintl`, GTK 3, and MIME/pixbuf data in
    the QEMU rootfs
  - includes Mesa EGL/GLES, Gallium software drivers, and `pciutils-libs` so
    Waterfox glxtest no longer fails on missing EGL/libpci runtime libraries
  - mounts `/run` and `/tmp` as tmpfs in the guest init
  - keeps the Waterfox profile under `/run/wfx-profile` for repeatable boots
  - loads input and virtio network modules, starts udev, triggers device
    discovery, and acquires DHCP on the first non-loopback interface
  - disables Gecko subprocess sandboxes for the QEMU proof
  - disables TRR/DoH and ORB JavaScript validation in the kiosk profile to keep
    networking native and avoid the debug utility-process assertion
  - disables chrome/content console-to-stdout prefs to keep debug-build
    `console.debug` output off the serial log
  - leaves WaterfoxBlocker disabled in the QEMU kiosk profile as a runtime
    fallback
- `docker/waterfox-musl/qemu-run`
  - defaults to plain serial stdio and no QEMU monitor
  - defaults to virtio keyboard/tablet input; `WFX_QEMU_INPUT=usb` or `both`
    are available for comparison
  - defaults to QEMU user networking; `WFX_QEMU_NETWORK=none` disables it
  - defaults the virtio GPU mode to 1600x1000 via `WFX_QEMU_WIDTH` and
    `WFX_QEMU_HEIGHT`
  - traps host `INT`/`TERM` and terminates the QEMU process
- `docker/waterfox-musl/wfx-musl`
  - passes `WFX_FORCE_REBUILD` through to the Alpine container; use it when
    changing cached source-built dependencies such as the custom cage/wlroots
    compositor
- `browser/locales/en-US/browser/waterfox.ftl`
  - restores the `browser/waterfox.ftl` resource that Waterfox browser chrome
    loads unconditionally; without it, typing in the URL bar hit
    `selectedBrowser` / `UrlbarInput` exceptions after localization bundle
    failures

## Cache And Artifact Layout

Generated content lives under `.wfx-cache/` and is not tracked.

- `.wfx-cache/apk`: APK cache
- `.wfx-cache/sources`: source tarballs
- `.wfx-cache/build`: unpacked/intermediate dependency builds
- `.wfx-cache/build-deps`: build-only source-built deps, such as mimalloc v3
- `.wfx-cache/sysroot`: runtime sysroot mounted as `/opt/wfx/sysroot`
- `.wfx-cache/mozbuild`: Gecko build state
- `.wfx-cache/sccache`: compiler cache
- `.wfx-cache/cargo`: Cargo cache
- `.wfx-cache/obj-aarch64-alpine-linux-musl`: current Waterfox object dir
- `.wfx-cache/dist`: staged Waterfox root, package, manifests, smoke reports
- `.wfx-cache/kiosk-compositor`: custom DRM/libinput cage/wlroots install
- `.wfx-cache/qemu`: QEMU rootfs, kernel, initramfs, and disk image

The squashed tree may not initially have all caches. If needed, reuse caches
from `/Users/josh/d/waterfox-musl/.wfx-cache/`, but copy only what is needed for
the task. For QEMU-only iteration, `dist/stage1-root` and `kiosk-compositor`
are enough.

The squashed `/tmp` checkout may also have `waterfox/browser/locales` as an
empty gitlink. `./mach configure` needs `waterfox/browser/locales/moz.build`;
initialize the submodule or restore that tiny metadata file before rerunning
configure.

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

Configure check:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl configure
```

Debug build:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl build
```

Package and static dependency scan:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl package
```

Rerun the final staged Waterfox ELF dependency check without repackaging:

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
WFX_QEMU_ACCEL=hvf docker/waterfox-musl/wfx-musl qemu-run
```

Bounded run for automated/local sanity checks:

```sh
timeout 180s env WFX_QEMU_DISPLAY=cocoa WFX_QEMU_ACCEL=hvf docker/waterfox-musl/wfx-musl qemu-run
```

Serial-only proof:

```sh
timeout 90s env WFX_QEMU_DISPLAY=none WFX_QEMU_ACCEL=hvf docker/waterfox-musl/wfx-musl qemu-run
```

## Development Loop

Prefer the narrowest loop that exercises the code you touched.

1. For Gecko source/config changes, run `configure` first if the change affects
   build flags, then `build`, then `package`.
2. For runtime sysroot changes, run `sysroot`, `check`, and `sysroot-smoke`
   before rebuilding Waterfox unless the change only affects QEMU packaging.
3. For headless compositor or browser runtime smoke changes, run
   `smoke-headless`. It is much faster than QEMU and catches loader, GTK, and
   Wayland surface regressions.
4. For QEMU rootfs/init changes, run `qemu-image`, then a bounded serial-only
   `qemu-run`. Only use Cocoa once serial shows seatd, wlroots, and Waterfox
   surfaces.
5. Keep `WFX_JOBS=8 WFX_CARGO_JOBS=8` while OrbStack has sufficient memory.
   If memory pressure returns, reduce jobs before changing code.

Expected fast-path timings after caches are warm:

- incremental build after a tiny C++ fix: seconds to minutes
- `package`: slower than build because tar/xz and staging are mostly serial
- `qemu-image`: usually tens of seconds after APK/cache warmup
- QEMU boot to Waterfox surfaces: roughly 20-30 seconds

## Known Build And Runtime Issues

Relrhack and packed relative relocations are disabled. The earlier path emitted
Android packed relocation dynamic tags; Alpine musl did not apply those during
`dlopen`, which crashed during NSPR initialization. Do not re-enable without
testing with `llvm-readelf` and a packaged runtime smoke.

Stage 1 uses clang/LLVM 22 for compile/link but `clang21-libclang` for bindgen.
This is an intentional temporary relaxation for the Waterfox 140 style binding
generator issue.

Stage 1 omits the built-in Rust client-certificate modules
`ipcclientcerts`, `osclientcerts`, and the `rsclientcerts` Rust test target.
Normal NSS verification and `trust-anchors` remain in the graph.

Debug Waterfox hits useful assertions. Fix narrowly and preserve upstream
semantics where possible. Recent examples:

- Servo flag reads/writes needed explicit atomic helpers for traversal.
- `nsNativeMenuService` destruction needed to tolerate `Init()` failing before
  singleton assignment in the no-DBus/native-menu runtime.
- Mimalloc debug arena assertion was guarded for the static mimalloc replacement
  path.

QEMU-specific issues already solved:

- direct kernel boot could not see `/dev/vda`; `qemu-image` now creates
  `initramfs-virt`
- `apk --no-scripts` left BusyBox applets missing; `qemu-image` now installs
  applet symlinks with `busybox --install -s`
- cage needed `libgcc_s.so.1`; Waterfox needed `libstdc++.so.6`; GTK needed
  `libintl.so.8`
- Alpine `seatd` does not support the earlier `-s` socket option
- guest profile data must not live on the root image; use `/run/wfx-profile`
- QEMU defaults to user-mode networking and serial boot verifies DHCP lease
  `10.0.2.15` from `10.0.2.2`
- Cage debugoptimized builds used to force wlroots debug logs, which produced
  repeated `Direct scan-out disabled by software cursor` output. The Cage patch
  now defaults runtime wlroots logging to errors while preserving `-D` as the
  explicit debug opt-in.
- Waterfox glxtest no longer reports missing `libpci` or `libEGL` after adding
  Mesa EGL/GLES, GBM, Gallium, and `pciutils-libs` to the QEMU rootfs.
- The visible QEMU compositor is now built with wlroots GLES2 rendering and the
  GBM allocator. The serial smoke needs `WLR_RENDERER_ALLOW_SOFTWARE=1` because
  QEMU virtio-gpu exposes software EGL in this proof environment.
- The ORB JavaScript validator is disabled in the kiosk profile to avoid a
  debug-only JSOracle utility-process assertion in this musl runtime.
- Stage 1 passes `--disable-waterfox-blocker`; the blocker component should not
  be built or registered for the musl debug build. The QEMU kiosk profile also
  keeps blocker prefs disabled as a runtime fallback.

Current noisy but nonfatal QEMU output:

- repeated Gecko debug-build `PuppetWidget without Tab` warnings
- Waterfox bundled sidebar extension JavaScript errors around
  `handle-autoplay-blocking.js`
- occasional WebRender texture upload warnings during active repaint

Current QEMU functional followup:

- Virtio keyboard/tablet now creates `/dev/input/event0` and `event1` in the
  guest. The missing manual input was caused by starting only the wlroots DRM
  backend. With `WLR_BACKENDS=drm,libinput`, serial boot verifies wlroots opens
  both event devices and adds the QEMU Virtio Keyboard and QEMU Virtio Tablet.
- Mouse input works in the Cocoa window.
- Keyboard input works in the Cocoa window. The later issue was that typed URL
  bar text did not render back even though Enter submitted the URL. Hamburger
  and context menus also appear as narrow blank or garbled surfaces. The
  compositor moved from pixman-only to GLES2/GBM, but the issue still
  reproduces on `hvf + virtio`; the QEMU image now disables EGL
  buffer-age/partial-update exposure, disables WebRender partial present, and
  pins Mesa to `kms_swrast` plus Gallium to llvmpipe. It now defaults wlroots
  to the pixman renderer to test stale-damage, compositor GLES2, and virtio
  GL-driver paths. Use `WFX_QEMU_GUEST_WLR_RENDERER=gles2` to restore the GLES2
  compositor path without editing the image.

If the Cocoa window is blank, check the launch line first. `hvf` should use
`gpu=virtio`; `tcg` should use `gpu=bochs`. `WFX_QEMU_ACCEL=tcg
WFX_QEMU_GPU=bochs` visibly renders the browser UI, while `hvf + bochs +
Cocoa` can leave the host window black even though QEMU `screendump` captures a
valid Waterfox framebuffer. Treat `hvf + bochs` as a diagnostic-only path and
set `WFX_QEMU_ALLOW_HVF_BOCHS=1` only when intentionally reproducing it.

On `hvf + virtio`, keep `virtio_gpu: driver missing` and
`webrender::device::gl` texture-crop warnings in the suspect set while menus or
small text updates fail to repaint. The DRI driver symlinks exist in the image,
so that log is more likely Mesa probing or rejecting a path than a simple
missing-file error.

## Dependency And Rejection Rules

The stage 1 browser runtime must reject X11/XCB/GLX/Xwayland, Vulkan, audio
stacks, DBus, portals, notification/secret services, speechd, cups, and dynamic
`libmimalloc.so`.

Static scans are in `check-waterfox-deps`, `package-stage1`,
`build-test-compositor`, `build-kiosk-compositor`, and `check-config`. The
final staged Waterfox scan compares every NEEDED entry against
`docker/waterfox-musl/waterfox-allowed-needed.txt` exactly and still hard-fails
on X11/XCB/GLX-family dependencies. Runtime smoke also scans loaded libraries
from `/proc/*/maps`.

Do not use Alpine stock `gtk+3.0` for the final runtime because it pulls X
libraries. Do not use Alpine stock `cage` for the QEMU proof because its
wlroots stack pulls unwanted XCB/Vulkan dependencies.

## Debugging Tips

Use `rg` first for code and script searches.

Useful logs and reports:

- `.wfx-cache/build/smoke-headless/cage-waterfox.log`
- `.wfx-cache/build/smoke-headless/loaded-libraries.txt`
- `.wfx-cache/dist/*.manifest.txt`
- `.wfx-cache/dist/*.headless-smoke.txt`
- `.wfx-cache/dist/*deps.txt`
- QEMU serial output from `qemu-run`

For dependency resolution inside the assembled QEMU rootfs:

```sh
docker run --rm --platform linux/arm64 \
  --volume "$PWD/.wfx-cache:/cache" \
  waterfox-musl:edge-clang22 \
  chroot /cache/qemu/rootfs /bin/sh -euc \
  'export LD_LIBRARY_PATH=/opt/wfx/kiosk-compositor/lib:/opt/waterfox:/opt/wfx/sysroot/lib; ldd /opt/waterfox/waterfox; ldd /opt/wfx/kiosk-compositor/bin/cage'
```

For QEMU command debugging, environment knobs are:

- `WFX_QEMU_DISPLAY`, default `cocoa`
- `WFX_QEMU_ACCEL`, default `hvf`
- `WFX_QEMU_GPU`, default `virtio` with `hvf` and `bochs` with `tcg`; accepted
  values are `virtio` and `bochs`
- `WFX_QEMU_MEMORY`, default `4096`
- `WFX_QEMU_SMP`, default `4`
- `WFX_QEMU_INPUT`, default `virtio`; accepted values are `virtio`, `usb`,
  `both`, and `none`
- `WFX_QEMU_WIDTH`, default `1600`
- `WFX_QEMU_HEIGHT`, default `1000`
- `WFX_QEMU_SERIAL`, default `stdio`
- `WFX_QEMU_MONITOR`, default `none`
- `WFX_QEMU_IMAGE`, `WFX_QEMU_KERNEL`, `WFX_QEMU_INITRAMFS`
- `WFX_QEMU_GUEST_WLR_RENDERER`, optional guest override for `WLR_RENDERER`
- `WFX_QEMU_GUEST_GALLIUM_DRIVER`, optional guest override for `GALLIUM_DRIVER`
- `WFX_QEMU_GUEST_MESA_LOADER`, optional guest override for
  `MESA_LOADER_DRIVER_OVERRIDE`

## When Updating The Plan

Keep `PLAN.md` as the user-facing project plan and status ledger. Update it
when a tranche status changes, when a relaxation is added, or when a previously
known blocker is fixed. Avoid dumping command output into the plan; record the
interpretation, artifact paths, and next action.

Before claiming the plan is complete, verify:

- debug build passed
- package and static scans passed
- headless Wayland smoke passed
- WaterfoxBlocker is excluded from the musl debug build
- QEMU image rebuilt from the current staged root
- QEMU boots twice in a row without profile/rootfs panics
- Cocoa QEMU visibly renders Waterfox, not just serial surface creation
- `Ctrl-C` from a real terminal terminates QEMU cleanly
- keyboard/mouse input reaches the guest, or the input gap is explicitly tracked
- `PLAN.md` reflects the final status and any followups
